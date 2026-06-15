#include "board/BoardSystem.h"
#include "board/BoardTouch.h"

#include "drivers/touch/ft6336/ft6336.h"

namespace {

uint8_t gTouchAddress = Board::Config::TOUCH_I2C_ADDRESS;

bool probeTouchAddress(TwoWire &touchWire, uint8_t address) {
  touchWire.beginTransmission(address);
  return touchWire.endTransmission(true) == 0;
}

bool applyMonitorModeIfRequired(TwoWire &touchWire, uint8_t address) {
  if (!Board::Config::TOUCH_REQUIRES_MONITOR_MODE) {
    return true;
  }

  touchWire.beginTransmission(address);
  touchWire.write(Board::Config::TOUCH_MONITOR_MODE_REGISTER);
  touchWire.write(Board::Config::TOUCH_MONITOR_MODE_VALUE);
  return touchWire.endTransmission(true) == 0;
}

}  // namespace

namespace Board::Touch {

TwoWire &wire() { return Wire; }

void resetController() { Board::System::resetTouchController(); }

bool ready() { return Board::Config::PIN_TOUCH_IRQ < 0 || !digitalRead(Board::Config::PIN_TOUCH_IRQ); }

bool configure() {
  TwoWire &touchWire = wire();
  const uint8_t candidates[] = {Board::Config::TOUCH_I2C_ADDRESS, 0x15, 0x14, 0x38};
  for (uint8_t address : candidates) {
    if (!probeTouchAddress(touchWire, address)) {
      continue;
    }
    gTouchAddress = address;
    return applyMonitorModeIfRequired(touchWire, address);
  }

  return false;
}

size_t packetLength() { return Ft6336Touch::packetLength(); }

bool readPacket(uint8_t *buffer, size_t len) {
  return Ft6336Touch::readPacket(wire(), gTouchAddress, buffer, len);
}

bool decodePacket(const uint8_t *data, size_t len, BoardDrivers::Touch::Sample &sample) {
  return Ft6336Touch::decodePacket(data, len, sample);
}

}  // namespace Board::Touch
