#include "board/BoardSystem.h"
#include "board/BoardTouch.h"

#include <algorithm>

namespace {

uint8_t gTouchAddress = Board::Config::TOUCH_I2C_ADDRESS;
constexpr uint8_t kProbeAddresses[] = {0x15, 0x14, 0x38, 0x5A};
constexpr uint8_t kStatusRegister = 0x02;
constexpr uint8_t kCoordsRegister = 0x03;
constexpr size_t kPacketLength = 6;

bool probeTouchAddress(TwoWire &touchWire, uint8_t address) {
  touchWire.beginTransmission(address);
  return touchWire.endTransmission(true) == 0;
}

uint16_t clampPhysicalX(uint16_t x) {
  return std::min<uint16_t>(x, static_cast<uint16_t>(Board::Config::PANEL_NATIVE_WIDTH - 1));
}

uint16_t clampPhysicalY(uint16_t y) {
  return std::min<uint16_t>(y, static_cast<uint16_t>(Board::Config::PANEL_NATIVE_HEIGHT - 1));
}

}  // namespace

namespace Board::Touch {

TwoWire &wire() { return Wire; }

void resetController() { Board::System::resetTouchController(); }

bool ready() { return true; }

bool configure() {
  TwoWire &touchWire = wire();
  for (uint8_t address : kProbeAddresses) {
    if (!probeTouchAddress(touchWire, address)) {
      continue;
    }
    gTouchAddress = address;
    return true;
  }

  return false;
}

size_t packetLength() { return kPacketLength; }

bool readPacket(uint8_t *buffer, size_t len) {
  if (buffer == nullptr || len < kPacketLength) {
    return false;
  }

  TwoWire &touchWire = wire();
  uint8_t status[2] = {};
  touchWire.beginTransmission(gTouchAddress);
  touchWire.write(kStatusRegister);
  if (touchWire.endTransmission(false) != 0) {
    return false;
  }

  if (touchWire.requestFrom(static_cast<uint8_t>(gTouchAddress), static_cast<size_t>(2), true) != 2) {
    return false;
  }

  status[0] = touchWire.read();
  status[1] = touchWire.read();
  const uint8_t touchCount = status[0];
  const uint8_t touchEvent = static_cast<uint8_t>(status[1] >> 6);

  // Waveshare CST820 examples treat event=1 as a non-press/release state.
  if (touchCount == 0 || touchEvent == 0x01) {
    for (size_t i = 0; i < kPacketLength; ++i) {
      buffer[i] = 0;
    }
    return true;
  }

  if (touchCount > 5) {
    return false;
  }

  touchWire.beginTransmission(gTouchAddress);
  touchWire.write(kCoordsRegister);
  if (touchWire.endTransmission(false) != 0) {
    return false;
  }

  if (touchWire.requestFrom(static_cast<uint8_t>(gTouchAddress), static_cast<size_t>(4), true) != 4) {
    return false;
  }

  buffer[0] = touchCount;
  buffer[1] = touchEvent;
  for (size_t i = 0; i < 4; ++i) {
    buffer[2 + i] = touchWire.read();
  }

  return true;
}

bool decodePacket(const uint8_t *data, size_t len, BoardDrivers::Touch::Sample &sample) {
  if (data == nullptr || len < kPacketLength) {
    return false;
  }

  if (data[0] == 0) {
    sample.touched = false;
    return true;
  }

  const uint16_t x = static_cast<uint16_t>(((data[2] & 0x0F) << 8) | data[3]);
  const uint16_t y = static_cast<uint16_t>(((data[4] & 0x0F) << 8) | data[5]);
  sample.touched = true;
  sample.physicalX = clampPhysicalX(x);
  sample.physicalY = clampPhysicalY(y);
  return true;
}

}  // namespace Board::Touch
