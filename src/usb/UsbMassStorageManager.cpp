#include "usb/UsbMassStorageManager.h"

#include <algorithm>
#include <cstring>

#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_partition.h>
#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
#include <wear_levelling.h>
#endif
#include <sdmmc_cmd.h>
#include <tusb.h>
#include <driver/sdmmc_host.h>

#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
#include <FFat.h>
#endif

#include "board/BoardConfig.h"

namespace {

constexpr uint16_t kUsbBlockSize = 512;
constexpr int kSdFrequenciesKhz[] = {
    SDMMC_FREQ_DEFAULT,
    10000,
    SDMMC_FREQ_PROBING,
};

static const char *kUsbMscTag = "usb_msc";

void deinitHostIfNeeded() {
  const esp_err_t err = sdmmc_host_deinit();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(kUsbMscTag, "SDMMC host deinit returned 0x%x", err);
  }
}

void pulseUsbReconnect() {
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  if (!tud_inited()) {
    return;
  }

  tud_disconnect();
  delay(120);
  tud_connect();
#endif
}

}  // namespace

UsbMassStorageManager *UsbMassStorageManager::instance_ = nullptr;

UsbMassStorageManager::UsbMassStorageManager() {
  instance_ = this;
  configureMsc();
}

bool UsbMassStorageManager::begin(bool writeEnabled) {
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  if (active_) {
    return true;
  }

  writeEnabled_ = writeEnabled;
  ejected_ = false;
  statusMessage_ = "Preparing storage";

  if (!beginSdCard()) {
    statusMessage_ = "Storage init failed";
    endSdCard();
    return false;
  }

  if (!configureMsc() || !msc_.begin(blockCount_, blockSize_)) {
    statusMessage_ = "USB MSC failed";
    endSdCard();
    return false;
  }

  active_ = true;
  statusMessage_ = writeEnabled_ ? "Mounted read/write" : "Mounted read-only";
  msc_.mediaPresent(true);
  pulseUsbReconnect();
  Serial.printf("[usb-msc] active blocks=%lu blockSize=%u write=%u\n",
                static_cast<unsigned long>(blockCount_), blockSize_, writeEnabled_ ? 1 : 0);
  return true;
#else
  (void)writeEnabled;
  statusMessage_ = "USB transfer disabled";
  Serial.println("[usb-msc] unsupported: build a USB-enabled target to enable it");
  return false;
#endif
}

void UsbMassStorageManager::end() {
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  msc_.mediaPresent(false);
  msc_.end();
#endif
  endSdCard();
  active_ = false;
  ejected_ = false;
  writeEnabled_ = false;
  statusMessage_ = "Idle";
}

bool UsbMassStorageManager::active() const { return active_; }

bool UsbMassStorageManager::ejected() const { return ejected_; }

bool UsbMassStorageManager::writeEnabled() const { return writeEnabled_; }

uint64_t UsbMassStorageManager::cardSizeBytes() const {
  return static_cast<uint64_t>(blockCount_) * blockSize_;
}

const char *UsbMassStorageManager::statusMessage() const { return statusMessage_; }

bool UsbMassStorageManager::configureMsc() {
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  msc_.vendorID("RSVPNANO");
  msc_.productID("File Transfer");
  msc_.productRevision("0.1");
  msc_.onRead(onRead);
  msc_.onWrite(onWrite);
  msc_.onStartStop(onStartStop);
  msc_.mediaPresent(false);
  return true;
#else
  return false;
#endif
}

bool UsbMassStorageManager::beginSdCard() {
#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  blockCount_ = 0;
  blockSize_ = kUsbBlockSize;
  flashEraseSize_ = 0;
  cardReady_ = false;

  if (flashPageBuffer_ == nullptr) {
    flashPageBuffer_ = static_cast<uint8_t *>(heap_caps_malloc(4096, MALLOC_CAP_INTERNAL));
  }
  if (flashPageBuffer_ == nullptr) {
    Serial.println("[usb-msc] failed to allocate WL sector buffer");
    return false;
  }

  flashPartition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT,
                                             "ffat");
  if (flashPartition_ == nullptr) {
    Serial.println("[usb-msc] flash partition 'ffat' not found");
    return false;
  }

  if (flashWlHandle_ != WL_INVALID_HANDLE) {
    wl_unmount(flashWlHandle_);
    flashWlHandle_ = WL_INVALID_HANDLE;
  }

  const esp_err_t wlErr = wl_mount(flashPartition_, &flashWlHandle_);
  if (wlErr != ESP_OK) {
    Serial.printf("[usb-msc] WL mount failed: 0x%x\n", wlErr);
    return false;
  }

  flashSectorSize_ = wl_sector_size(flashWlHandle_);
  if (flashSectorSize_ == 0) {
    Serial.println("[usb-msc] WL sector size is zero");
    return false;
  }
  if ((flashSectorSize_ & (flashSectorSize_ - 1)) != 0) {
    Serial.printf("[usb-msc] WL sector size not power-of-two: %u\n",
                  static_cast<unsigned int>(flashSectorSize_));
    return false;
  }
  flashEraseSize_ = static_cast<uint32_t>(flashSectorSize_);

  const size_t usableSize = wl_size(flashWlHandle_);
  blockCount_ = static_cast<uint32_t>(usableSize / blockSize_);
  cardReady_ = true;
  Serial.printf("[usb-msc] flash ready for USB (%lu MB)\n",
                static_cast<unsigned long>(cardSizeBytes() / (1024ULL * 1024ULL)));
  return true;
#else
  blockCount_ = 0;
  blockSize_ = kUsbBlockSize;
  cardReady_ = false;

  if (sectorBuffer_ == nullptr) {
    sectorBuffer_ = static_cast<uint8_t *>(
        heap_caps_malloc(kUsbBlockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  }
  if (sectorBuffer_ == nullptr) {
    Serial.println("[usb-msc] failed to allocate DMA sector buffer");
    return false;
  }

  sdmmc_slot_config_t slotConfig = SDMMC_SLOT_CONFIG_DEFAULT();
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
  slotConfig.clk = static_cast<gpio_num_t>(Board::Config::PIN_SD_CLK);
  slotConfig.cmd = static_cast<gpio_num_t>(Board::Config::PIN_SD_CMD);
  slotConfig.d0 = static_cast<gpio_num_t>(Board::Config::PIN_SD_D0);
  slotConfig.d1 = GPIO_NUM_NC;
  slotConfig.d2 = GPIO_NUM_NC;
  slotConfig.d3 = GPIO_NUM_NC;
#endif
  slotConfig.width = 1;

  for (int frequencyKhz : kSdFrequenciesKhz) {
    std::memset(&card_, 0, sizeof(card_));
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.slot = SDMMC_HOST_SLOT_1;
    host.max_freq_khz = frequencyKhz;

    deinitHostIfNeeded();
    esp_err_t err = host.init();
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] host init failed at %d kHz: 0x%x\n", frequencyKhz, err);
      continue;
    }

    err = sdmmc_host_init_slot(host.slot, &slotConfig);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] slot init failed at %d kHz: 0x%x\n", frequencyKhz, err);
      deinitHostIfNeeded();
      continue;
    }

    err = sdmmc_card_init(&host, &card_);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] card init failed at %d kHz: 0x%x\n", frequencyKhz, err);
      deinitHostIfNeeded();
      continue;
    }

    if (card_.csd.sector_size != kUsbBlockSize || card_.csd.capacity == 0) {
      Serial.printf("[usb-msc] unsupported SD geometry: sectors=%d sectorSize=%d\n",
                    card_.csd.capacity, card_.csd.sector_size);
      deinitHostIfNeeded();
      continue;
    }

    blockCount_ = static_cast<uint32_t>(card_.csd.capacity);
    blockSize_ = static_cast<uint16_t>(card_.csd.sector_size);
    cardReady_ = true;
    Serial.printf("[usb-msc] SD ready for USB at %d kHz (%lu MB)\n", frequencyKhz,
                  static_cast<unsigned long>(cardSizeBytes() / (1024ULL * 1024ULL)));
    return true;
  }

  return false;
#endif
}

void UsbMassStorageManager::endSdCard() {
#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  if (flashWlHandle_ != WL_INVALID_HANDLE) {
    wl_unmount(flashWlHandle_);
    flashWlHandle_ = WL_INVALID_HANDLE;
  }
  flashPartition_ = nullptr;
  cardReady_ = false;
  blockCount_ = 0;
  flashSectorSize_ = 0;
  if (flashPageBuffer_ != nullptr) {
    heap_caps_free(flashPageBuffer_);
    flashPageBuffer_ = nullptr;
  }
#else
  if (cardReady_) {
    deinitHostIfNeeded();
  }
  cardReady_ = false;
  blockCount_ = 0;

  if (sectorBuffer_ != nullptr) {
    heap_caps_free(sectorBuffer_);
    sectorBuffer_ = nullptr;
  }
#endif
}

int32_t UsbMassStorageManager::onRead(uint32_t lba, uint32_t offset, void *buffer,
                                      uint32_t bufsize) {
  if (instance_ == nullptr) {
    return -1;
  }
  return instance_->readSectors(lba, offset, buffer, bufsize);
}

int32_t UsbMassStorageManager::onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer,
                                       uint32_t bufsize) {
  if (instance_ == nullptr) {
    return -1;
  }
  return instance_->writeSectors(lba, offset, buffer, bufsize);
}

bool UsbMassStorageManager::onStartStop(uint8_t powerCondition, bool start, bool loadEject) {
  if (instance_ == nullptr) {
    return false;
  }
  return instance_->handleStartStop(powerCondition, start, loadEject);
}

int32_t UsbMassStorageManager::readSectors(uint32_t lba, uint32_t offset, void *buffer,
                                           uint32_t bufsize) {
  if (!active_ || !cardReady_ || buffer == nullptr || offset >= blockSize_) {
    return -1;
  }

#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  const uint64_t absoluteStart = (static_cast<uint64_t>(lba) * blockSize_) + offset;
  const uint64_t absoluteEnd = absoluteStart + bufsize;
  const uint64_t usableSize = wl_size(flashWlHandle_);
  if (absoluteStart >= usableSize) {
    return -1;
  }
  const uint64_t cappedEnd = std::min<uint64_t>(absoluteEnd, usableSize);
  const size_t readSize = static_cast<size_t>(cappedEnd - absoluteStart);
  const esp_err_t err = wl_read(flashWlHandle_, absoluteStart, buffer, readSize);
  if (err != ESP_OK) {
    Serial.printf("[usb-msc] WL read failed offset=%llu err=0x%x\n",
                  static_cast<unsigned long long>(absoluteStart), err);
    return -1;
  }
  return static_cast<int32_t>(readSize);
#else
  uint8_t *out = static_cast<uint8_t *>(buffer);
  uint32_t copied = 0;
  uint32_t currentLba = lba;
  uint32_t currentOffset = offset;

  while (copied < bufsize && currentLba < blockCount_) {
    const uint32_t bytesThisSector =
        std::min<uint32_t>(blockSize_ - currentOffset, bufsize - copied);
    const esp_err_t err = sdmmc_read_sectors(&card_, sectorBuffer_, currentLba, 1);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] read failed lba=%lu err=0x%x\n",
                    static_cast<unsigned long>(currentLba), err);
      return copied > 0 ? static_cast<int32_t>(copied) : -1;
    }

    std::memcpy(out + copied, sectorBuffer_ + currentOffset, bytesThisSector);
    copied += bytesThisSector;
    currentOffset = 0;
    ++currentLba;
  }

  return static_cast<int32_t>(copied);
#endif
}

int32_t UsbMassStorageManager::writeSectors(uint32_t lba, uint32_t offset, uint8_t *buffer,
                                            uint32_t bufsize) {
  if (!writeEnabled_) {
    return -1;
  }
  if (!active_ || !cardReady_ || buffer == nullptr || offset >= blockSize_) {
    return -1;
  }

#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  if (flashWlHandle_ == WL_INVALID_HANDLE || flashPageBuffer_ == nullptr) {
    return -1;
  }

  uint32_t written = 0;
  while (written < bufsize) {
    const uint64_t absolute = (static_cast<uint64_t>(lba) * blockSize_) + offset + written;
    const uint64_t usableSize = wl_size(flashWlHandle_);
    if (absolute >= usableSize) {
      return written > 0 ? static_cast<int32_t>(written) : -1;
    }

    const uint64_t pageStart = (absolute / flashEraseSize_) * flashEraseSize_;
    const uint32_t pageOffset = static_cast<uint32_t>(absolute - pageStart);
    const uint32_t bytesThisPage =
        std::min<uint32_t>(flashEraseSize_ - pageOffset, bufsize - written);

    esp_err_t err = wl_read(flashWlHandle_, pageStart, flashPageBuffer_, flashEraseSize_);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] WL pre-read failed offset=%llu err=0x%x\n",
                    static_cast<unsigned long long>(pageStart), err);
      return written > 0 ? static_cast<int32_t>(written) : -1;
    }

    std::memcpy(flashPageBuffer_ + pageOffset, buffer + written, bytesThisPage);

    err = wl_erase_range(flashWlHandle_, pageStart, flashEraseSize_);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] WL erase failed offset=%llu err=0x%x\n",
                    static_cast<unsigned long long>(pageStart), err);
      return written > 0 ? static_cast<int32_t>(written) : -1;
    }

    err = wl_write(flashWlHandle_, pageStart, flashPageBuffer_, flashEraseSize_);
    if (err != ESP_OK) {
      Serial.printf("[usb-msc] WL write failed offset=%llu err=0x%x\n",
                    static_cast<unsigned long long>(pageStart), err);
      return written > 0 ? static_cast<int32_t>(written) : -1;
    }

    written += bytesThisPage;
  }

  return static_cast<int32_t>(written);
#else
  uint32_t written = 0;
  uint32_t currentLba = lba;
  uint32_t currentOffset = offset;

  while (written < bufsize && currentLba < blockCount_) {
    const uint32_t bytesThisSector =
        std::min<uint32_t>(blockSize_ - currentOffset, bufsize - written);
    if (currentOffset != 0 || bytesThisSector != blockSize_) {
      const esp_err_t readErr = sdmmc_read_sectors(&card_, sectorBuffer_, currentLba, 1);
      if (readErr != ESP_OK) {
        Serial.printf("[usb-msc] write pre-read failed lba=%lu err=0x%x\n",
                      static_cast<unsigned long>(currentLba), readErr);
        return written > 0 ? static_cast<int32_t>(written) : -1;
      }
    }

    std::memcpy(sectorBuffer_ + currentOffset, buffer + written, bytesThisSector);

    const esp_err_t writeErr = sdmmc_write_sectors(&card_, sectorBuffer_, currentLba, 1);
    if (writeErr != ESP_OK) {
      Serial.printf("[usb-msc] write failed lba=%lu err=0x%x\n",
                    static_cast<unsigned long>(currentLba), writeErr);
      return written > 0 ? static_cast<int32_t>(written) : -1;
    }

    written += bytesThisSector;
    currentOffset = 0;
    ++currentLba;
  }

  return static_cast<int32_t>(written);
#endif
}

bool UsbMassStorageManager::handleStartStop(uint8_t powerCondition, bool start, bool loadEject) {
  Serial.printf("[usb-msc] start-stop power=%u start=%u eject=%u\n", powerCondition,
                start ? 1 : 0, loadEject ? 1 : 0);

  if (loadEject && !start) {
    ejected_ = true;
    statusMessage_ = "Ejected";
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
    msc_.mediaPresent(false);
#endif
  }

  return true;
}
