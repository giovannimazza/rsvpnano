#include "board/BoardPower.h"

#include <Wire.h>
#include <driver/gpio.h>
#include <algorithm>

#include "drivers/power/BatteryCurve.h"
#include "drivers/power/axp2101/Axp2101.h"

namespace {

struct PowerContext {
  bool batteryPowerHoldEnabled = false;
  bool axp2101Detected = false;
  bool axp2101Probed = false;
};

PowerContext gPower;

bool probeAxp2101() {
  constexpr uint8_t kAxpAddress = 0x34;
  Wire.beginTransmission(kAxpAddress);
  const uint8_t err = Wire.endTransmission(true);
  const bool found = (err == 0);
  Serial.printf("[board] AXP2101 probe addr=0x%02X err=%u found=%u\n", kAxpAddress, err, found ? 1 : 0);
  return found;
}

}  // namespace

namespace Board::Power {

void begin() {
  if (Config::PIN_BATTERY_HOLD >= 0) {
    const gpio_num_t batteryHoldPin = static_cast<gpio_num_t>(Config::PIN_BATTERY_HOLD);
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(batteryHoldPin);
    pinMode(Config::PIN_BATTERY_HOLD, OUTPUT);
    digitalWrite(Config::PIN_BATTERY_HOLD, HIGH);
    gPower.batteryPowerHoldEnabled = true;
  }

  if (Config::PIN_BATTERY_ADC >= 0) {
    pinMode(Config::PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(Config::PIN_BATTERY_ADC, ADC_11db);
  }
}

void prepareDeepSleepPowerHold() {
  if (Config::PIN_BATTERY_HOLD < 0) {
    return;
  }

  const gpio_num_t batteryHoldPin = static_cast<gpio_num_t>(Config::PIN_BATTERY_HOLD);
  pinMode(Config::PIN_BATTERY_HOLD, OUTPUT);
  digitalWrite(Config::PIN_BATTERY_HOLD, HIGH);
  gpio_set_direction(batteryHoldPin, GPIO_MODE_OUTPUT);
  gpio_set_level(batteryHoldPin, 1);
  gpio_hold_en(batteryHoldPin);
  gpio_deep_sleep_hold_en();
}

void resetWakePeripherals() {}

bool enableAudioPowerIfAvailable() { return false; }

bool readBatteryStatus(BatteryStatus &status) {
  status = BatteryStatus{};

  // On first call, probe for AXP2101 on the Wire bus.
  if (!gPower.axp2101Probed) {
    gPower.axp2101Probed = true;
    gPower.axp2101Detected = probeAxp2101();
    if (gPower.axp2101Detected) {
      BoardDrivers::Axp2101::begin();
    }
  }

  if (gPower.axp2101Detected) {
    return BoardDrivers::Axp2101::readBatteryStatus(status);
  }

  if (Config::PIN_BATTERY_ADC < 0) {
    return false;
  }

  constexpr uint8_t kMaxSamples = 24;
  constexpr uint8_t kRawSamples = 16;
  constexpr float kBatteryDividerRatio = 3.0f;
  constexpr float kBatteryVoltageOffset = 0.0f;

  delay(12);
  uint32_t millivolts[kMaxSamples];
  uint8_t samples = 0;
  for (uint8_t i = 0; i < kMaxSamples + 2; ++i) {
    const uint32_t sample = analogReadMilliVolts(Config::PIN_BATTERY_ADC);
    if (i >= 2 && sample > 0 && samples < kMaxSamples) {
      millivolts[samples++] = sample;
    }
    delayMicroseconds(500);
  }

  float pinMillivolts = 0.0f;
  if (samples == 0) {
    uint32_t rawTotal = 0;
    for (uint8_t i = 0; i < kRawSamples; ++i) {
      rawTotal += analogRead(Config::PIN_BATTERY_ADC);
      delayMicroseconds(500);
    }
    pinMillivolts =
        (static_cast<float>(rawTotal) / static_cast<float>(kRawSamples)) * 3300.0f / 4095.0f;
  } else {
    std::sort(millivolts, millivolts + samples);
    const uint8_t trim = samples >= 10 ? 2 : 0;
    uint32_t trimmedTotal = 0;
    uint8_t trimmedSamples = 0;
    for (uint8_t i = trim; i < samples - trim; ++i) {
      trimmedTotal += millivolts[i];
      ++trimmedSamples;
    }
    pinMillivolts =
        static_cast<float>(trimmedTotal) / static_cast<float>(std::max<uint8_t>(1, trimmedSamples));
  }

  status.voltage = (pinMillivolts * kBatteryDividerRatio / 1000.0f) + kBatteryVoltageOffset;
  Serial.printf("[board] battery ADC pin=%d samples=%u pin_mv=%.1f voltage=%.2fV\n",
                Config::PIN_BATTERY_ADC, samples, static_cast<double>(pinMillivolts),
                static_cast<double>(status.voltage));

  status.present = status.voltage >= 2.5f && status.voltage <= 4.6f;
  if (!status.present) {
    status.percent = 0;
    return false;
  }

  status.percent = BoardDrivers::BatteryCurve::percentForVoltage(status.voltage);
  return true;
}

DiagnosticSnapshot diagnosticSnapshot() {
  if (gPower.axp2101Detected) {
    return BoardDrivers::Axp2101::diagnosticSnapshot();
  }
  return PowerDiagnosticSnapshot{};
}

bool externalPowerPresent() {
  if (gPower.axp2101Detected) {
    return BoardDrivers::Axp2101::externalPowerPresent();
  }
  return false;
}

bool releaseBatteryPowerHold() {
  if (Config::PIN_BATTERY_HOLD < 0) {
    return false;
  }

  digitalWrite(Config::PIN_BATTERY_HOLD, LOW);
  gPower.batteryPowerHoldEnabled = false;
  Serial.println("[board] Battery power hold released");
  return true;
}

bool powerOffUsesControllerWake() { return false; }

bool shouldRequestShutdownOnPowerOff() { return Config::REQUEST_PMU_SHUTDOWN_ON_POWEROFF; }

bool shouldReleaseBatteryPowerBeforeDeepSleep() {
  return Config::RELEASE_BATTERY_HOLD_BEFORE_DEEP_SLEEP;
}

}  // namespace Board::Power
