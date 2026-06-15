#pragma once

#include <Arduino.h>
#include <sdkconfig.h>

#ifndef RSVP_USB_TRANSFER_ENABLED
#define RSVP_USB_TRANSFER_ENABLED 0
#endif

#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
#include <USBMSC.h>
#endif

#include <esp_partition.h>
#include <driver/sdmmc_types.h>

class UsbMassStorageManager {
 public:
  UsbMassStorageManager();

  bool begin(bool writeEnabled);
  void end();
  bool active() const;
  bool ejected() const;
  bool writeEnabled() const;
  uint64_t cardSizeBytes() const;
  const char *statusMessage() const;

 private:
  static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize);
  static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize);
  static bool onStartStop(uint8_t powerCondition, bool start, bool loadEject);

  bool beginSdCard();
  void endSdCard();
  bool configureMsc();
  int32_t readSectors(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize);
  int32_t writeSectors(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize);
  bool handleStartStop(uint8_t powerCondition, bool start, bool loadEject);
  bool beginFlashStorage();
  void endFlashStorage();

  static UsbMassStorageManager *instance_;

#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  USBMSC msc_;
#endif

  sdmmc_card_t card_ = {};
  const esp_partition_t *flashPartition_ = nullptr;
  uint8_t *sectorBuffer_ = nullptr;
  uint8_t *flashPageBuffer_ = nullptr;
  uint32_t blockCount_ = 0;
  uint16_t blockSize_ = 512;
  uint32_t flashEraseSize_ = 4096;
  bool active_ = false;
  bool cardReady_ = false;
  bool ejected_ = false;
  bool writeEnabled_ = false;
  const char *statusMessage_ = "Idle";
};
