#include "board/BoardSystem.h"
#include "board/BoardTouch.h"

#include <algorithm>

namespace {

uint8_t gTouchAddress = Board::Config::TOUCH_I2C_ADDRESS;
constexpr uint8_t kProbeAddresses[] = {0x15, 0x14, 0x38};
constexpr uint8_t kCountRegister = 2;
constexpr uint8_t kPointRegister = 3;

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

bool ready() { return Board::Config::PIN_TOUCH_IRQ < 0 || !digitalRead(Board::Config::PIN_TOUCH_IRQ); }

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

size_t packetLength() { return 7; }

bool readPacket(uint8_t *buffer, size_t len) {
  if (buffer == nullptr || len < packetLength()) {
    return false;
  }

  TwoWire &touchWire = wire();
  touchWire.beginTransmission(gTouchAddress);
  touchWire.write(kCountRegister);
  if (touchWire.endTransmission(false) != 0) {
    return false;
  }

  if (touchWire.requestFrom(static_cast<uint8_t>(gTouchAddress), static_cast<size_t>(1), true) != 1) {
    return false;
  }

  const uint8_t count = touchWire.read();
  buffer[0] = count;
  if (count == 0) {
    for (size_t i = 1; i < packetLength(); ++i) {
      buffer[i] = 0;
    }
    return true;
  }

  if (count > 5) {
    return false;
  }

  touchWire.beginTransmission(gTouchAddress);
  touchWire.write(kPointRegister);
  if (touchWire.endTransmission(false) != 0) {
    return false;
  }

  if (touchWire.requestFrom(static_cast<uint8_t>(gTouchAddress), static_cast<size_t>(6), true) != 6) {
    return false;
  }

  for (size_t i = 0; i < 6; ++i) {
    buffer[i + 1] = touchWire.read();
  }

  return true;
}

bool decodePacket(const uint8_t *data, size_t len, BoardDrivers::Touch::Sample &sample) {
  if (data == nullptr || len < packetLength()) {
    return false;
  }

  const uint8_t count = data[0];
  if (count == 0) {
    sample.touched = false;
    return true;
  }

  const uint16_t x = static_cast<uint16_t>(((data[1] & 0x0F) << 8) | data[2]);
  const uint16_t y = static_cast<uint16_t>(((data[3] & 0x0F) << 8) | data[4]);
  sample.touched = true;
  sample.physicalX = clampPhysicalX(x);
  sample.physicalY = clampPhysicalY(y);
  return true;
}

}  // namespace Board::Touch
