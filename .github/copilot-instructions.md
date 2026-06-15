# Copilot instructions for rsvpnano

## Build, test, and release

- Firmware build: `pio run`
- Explicit firmware targets:
  - `pio run -e waveshare_esp32s3_usb_msc` (default; USB transfer enabled)
  - `pio run -e waveshare_esp32s3` (USB transfer disabled)
- For device-specific work, build the target that matches the hardware path you are touching and keep board code in the matching `src/platforms/<device>/` files:
  - `src/platforms/waveshare_amoled_18/`
  - `src/platforms/waveshare_amoled_216/`
  - `src/platforms/waveshare_amoled_241/`
  - `src/platforms/waveshare_lcd_349/rev1/`
  - `src/platforms/waveshare_lcd_349/rev2/`
- Upload to a connected device: `pio run -t upload`
- Serial monitor: `pio device monitor`
- Native tests: `pio test -e native_test`
- Run only the pacing suite: `pio test -e native_test -f test_pacing`
- Release: `.\tools\release.ps1 -Version v0.1.X-noogi`
- Export browser/OTA assets for a release: `python3 tools/export_web_firmware.py --version v0.0.5`

## High-level architecture

- `src/main.cpp` is the entrypoint: it starts the board layer and then hands control to `App::begin()` / `App::update()`.
- `src/app/App.cpp` is the top-level state machine. It owns menu flow, reader flow, power/standby behavior, settings persistence, OTA, RSS, companion sync, USB transfer, and focus timer orchestration.
- `src/display/DisplayManager.*` owns all rendering and screen layout. Most screens are drawn through this layer rather than directly from `App`.
- `src/reader/ReadingLoop.*` handles RSVP pacing, word timing, pause/seek/scrub behavior, and progress tracking.
- `src/storage/StorageManager.*` manages SD card access, indexed books, chapter markers, and book loading.
- `src/storage/EpubConverter.*` handles on-device EPUB conversion when enabled.
- `src/timer/FocusTimer.*` manages the focus timer and genre-specific touch durations.
- `src/sync/CompanionSyncManager.*` implements the browser/iOS companion sync path.
- `src/update/OtaUpdater.*` handles GitHub Releases-based firmware updates.
- `src/usb/UsbMassStorageManager.*` covers the optional USB MSC transfer mode.
- `src/board/BoardConfig.h` is the hardware contract: pins, display geometry, and board-level feature toggles.

## Key conventions

- Keep hardware constants and board-specific assumptions behind `BoardConfig` and the board/platform files under `src/platforms/` and `src/drivers/`.
- `App` is the orchestration layer; avoid spreading state-machine logic into lower-level modules.
- Settings are persisted through `Preferences` in `App.cpp`. NVS keys are short by design and must stay within ESP32 limits.
- Settings menus are built from index constants plus `rebuildSettingsMenuItems()` and `selectSettingsItem()`. When adding a setting, update the index, the menu builder, the handler, and the preference load/save path together.
- Reader modes are mode-aware: RSVP and Scroll have separate defaults and visibility rules, especially for chapter labels and footer chrome.
- `readerChrome()` controls footer visibility flags; chapter/battery/progress labels are mode- and setting-dependent.
- Keep display and input behavior routed through the existing helpers (`DisplayManager`, `Input::Touch`, `Input::Buttons`) instead of adding ad hoc hardware calls in `App`.
- Generated or release-time assets should stay tied to `tools/export_web_firmware.py` and the release workflow, not hand-edited.
