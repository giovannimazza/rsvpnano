#pragma once

#include "board/BoardTypes.h"

namespace Board::Config {

using UiOrientation = Board::UiOrientation;
using StorageBusKind = Board::StorageBusKind;
using PowerManagerKind = Board::PowerManagerKind;
using BatteryStatus = Board::BatteryStatus;
using PowerDiagnosticSnapshot = Board::PowerDiagnosticSnapshot;
constexpr const char *BOARD_ID = "waveshare_esp32s3_touch_amoled_143c";
constexpr const char *BOARD_LABEL = "Waveshare ESP32-S3-Touch-AMOLED-1.43C";
constexpr const char *OTA_ASSET_NAME = "rsvp-nano-esp32-s3-touch-amoled-1.43c-ota.bin";
constexpr StorageBusKind STORAGE_BUS = StorageBusKind::SdMmc1Bit;
constexpr PowerManagerKind POWER_MANAGER = PowerManagerKind::DirectGpioBatteryHold;
constexpr bool HAS_LCD_BACKLIGHT = false;
constexpr bool HAS_AUDIO_OUTPUT = true;
constexpr bool TOUCH_USES_WIRE1 = false;
constexpr bool HAS_IMU = false;
constexpr bool IMU_USES_WIRE1 = false;
constexpr bool IMU_RELEASE_BUS_BEFORE_READ = false;
constexpr uint8_t IMU_I2C_ADDRESS = 0x00;
constexpr bool SWAP_APP_BOOT_AND_POWER_BUTTONS = false;
constexpr bool APP_POWER_BUTTON_USES_PMU_EVENTS = false;
constexpr bool BOOT_BUTTON_WAKES_STANDBY = true;
constexpr bool ENABLE_TOP_EDGE_MENU_SWIPE = true;
constexpr bool ENABLE_BOTTOM_EDGE_QUICK_SETTINGS_SWIPE = true;
constexpr bool FIRMWARE_POWER_BUTTON_ENABLED = true;
constexpr bool BOOT_BUTTON_TOGGLES_READER = true;
constexpr bool BOOT_BUTTON_BACKS_OUT_OF_MENU = true;
constexpr bool BOOT_BUTTON_HOLD_STARTS_STANDBY = true;
constexpr bool ENABLE_RESTRUCTURED_MENU = true;

constexpr int PIN_BOOT_BUTTON = 0;
constexpr int PIN_PWR_BUTTON = 21;
constexpr int PIN_KEY_BUTTON = -1;
constexpr int PIN_BATTERY_ADC = 4;
constexpr int PIN_BATTERY_HOLD = -1;

constexpr int PIN_LCD_CS = 15;
constexpr int PIN_LCD_SCLK = 14;
constexpr int PIN_LCD_DATA0 = 9;
constexpr int PIN_LCD_DATA1 = 10;
constexpr int PIN_LCD_DATA2 = 11;
constexpr int PIN_LCD_DATA3 = 12;
constexpr int PIN_LCD_RST = 13;
constexpr int PIN_LCD_BACKLIGHT = -1;

constexpr int PANEL_NATIVE_WIDTH = 466;
constexpr int PANEL_NATIVE_HEIGHT = 466;
constexpr int DISPLAY_WIDTH = 466;
constexpr int DISPLAY_HEIGHT = 466;
constexpr int READER_CHROME_MARGIN_X = 40;
constexpr int READER_CHROME_MARGIN_TOP = 24;
constexpr int READER_CHROME_MARGIN_BOTTOM = 24;
constexpr int READER_BATTERY_MARGIN_X = 64;
constexpr int READER_BATTERY_MARGIN_TOP = 32;
constexpr int PIN_DEEP_SLEEP_WAKE = PIN_PWR_BUTTON;
constexpr bool SUPPORTS_SOFTWARE_POWEROFF = true;
constexpr bool RELEASE_BATTERY_HOLD_BEFORE_DEEP_SLEEP = false;
constexpr bool REQUEST_PMU_SHUTDOWN_ON_POWEROFF = false;
constexpr bool SOFTWARE_POWEROFF_USES_SOFT_LOOP = false;
constexpr bool SOFT_OFF_WAKE_USES_POWER_BUTTON = false;
constexpr bool SOFT_OFF_WAKE_USES_BOOT_BUTTON = false;
constexpr bool PMU_REQUIRES_POWER_KEY_CONFIG = false;
constexpr bool AXP2101_RELEASE_BUS_BEFORE_READ = false;
constexpr bool AXP2101_ENABLE_POWER_KEY_IRQS = false;
constexpr bool TCA9554_HAS_DISPLAY_SEQUENCE = false;
constexpr bool TCA9554_HAS_POWER_BUTTON = false;
constexpr bool TCA9554_RELEASE_BUS_BEFORE_READ = false;
constexpr uint8_t PMU_POWER_KEY_ON_TIME_VALUE = 0x00;
constexpr uint8_t PMU_POWER_KEY_OFF_TIME_VALUE = 0x00;
constexpr uint32_t PMU_BOOT_BUTTON_IGNORE_MS = 1200;
constexpr uint32_t SOFT_OFF_WAKE_CONFIRM_MS = 90;
constexpr uint32_t SYSTEM_I2C_CLOCK_HZ = 300000;
constexpr uint32_t SYSTEM_I2C_TIMEOUT_MS = 10;
constexpr uint32_t TOUCH_I2C_CLOCK_HZ = 100000;
constexpr uint32_t TOUCH_I2C_TIMEOUT_MS = 10;
constexpr size_t DISPLAY_TX_CHUNK_BYTES = 32 * 1024;
constexpr bool UI_ROTATED_180 = false;
constexpr UiOrientation DEFAULT_UI_ORIENTATION =
    UI_ROTATED_180 ? UiOrientation::LandscapeFlipped : UiOrientation::Landscape;
constexpr UiOrientation ROTATED_UI_ORIENTATION =
    UI_ROTATED_180 ? UiOrientation::Landscape : UiOrientation::LandscapeFlipped;

constexpr int PIN_SD_CLK = -1;
constexpr int PIN_SD_CMD = -1;
constexpr int PIN_SD_D0 = -1;
constexpr int PIN_I2C_SDA = 47;
constexpr int PIN_I2C_SCL = 48;
constexpr int PIN_TOUCH_SDA = 47;
constexpr int PIN_TOUCH_SCL = 48;
constexpr int PIN_TOUCH_IRQ = -1;
constexpr int PIN_TOUCH_RST = 16;
constexpr uint8_t TOUCH_I2C_ADDRESS = 0x15;
constexpr bool TOUCH_REQUIRES_MONITOR_MODE = false;
constexpr bool TOUCH_RELEASE_BUS_BEFORE_READ = false;
constexpr uint8_t TOUCH_MONITOR_MODE_REGISTER = 0x00;
constexpr uint8_t TOUCH_MONITOR_MODE_VALUE = 0x00;
constexpr uint32_t TOUCH_POLL_INTERVAL_MS = 20;
constexpr uint32_t TOUCH_FAILURE_BACKOFF_MS = 250;
constexpr uint32_t TOUCH_RECOVERY_RETRY_MS = 1000;
constexpr uint32_t TOUCH_RECOVERY_EVENT_IGNORE_MS = 0;

constexpr int TCA9554_ADDRESS = -1;
constexpr uint8_t TCA9554_PIN_PWR_BUTTON = 0;
constexpr uint8_t TCA9554_PIN_PMU_IRQ = 0;
constexpr uint8_t TCA9554_PIN_SD_ENABLE = 0;
constexpr uint8_t TCA9554_PIN_TOUCH_RESET = 0;
constexpr uint8_t TCA9554_PIN_LCD_RESET = 0;
constexpr uint8_t TCA9554_PIN_DISPLAY_ENABLE = 0;
constexpr uint8_t TCA9554_PIN_BATTERY_ADC_ENABLE = 0;
constexpr uint8_t TCA9554_PIN_SYS_EN = 0;
constexpr uint8_t TCA9554_PIN_AUDIO_ENABLE = 0;

constexpr int PIN_AUDIO_MCLK = 38;
constexpr int PIN_AUDIO_BCLK = 39;
constexpr int PIN_AUDIO_WS = 40;
constexpr int PIN_AUDIO_DIN = 41;
constexpr int PIN_AUDIO_DOUT = 42;
constexpr uint8_t ES8311_ADDRESS = 0x18;
}  // namespace Board::Config
