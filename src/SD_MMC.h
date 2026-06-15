#pragma once

#include "board/BoardConfig.h"

#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)

#include <FS.h>
#include <LittleFS.h>

static constexpr uint8_t CARD_NONE = 0;
static constexpr uint8_t CARD_MMC = 1;
static constexpr uint8_t CARD_SD = 2;
static constexpr uint8_t CARD_SDHC = 3;

class InternalSdMmcShim {
 public:
  bool setPins(int, int, int) { return true; }

  bool begin(const char * = nullptr, bool = true, bool = false, uint8_t = 0, uint8_t = 0) {
    if (mounted_) {
      return true;
    }

    mounted_ = LittleFS.begin(true);
    return mounted_;
  }

  void end() { mounted_ = false; }

  File open(const char *path) { return LittleFS.open(path); }
  File open(const char *path, const char *mode) { return LittleFS.open(path, mode); }
  File open(const String &path) { return LittleFS.open(path); }
  File open(const String &path, const char *mode) { return LittleFS.open(path, mode); }

  bool mkdir(const char *path) { return LittleFS.mkdir(path); }
  bool mkdir(const String &path) { return LittleFS.mkdir(path); }
  bool remove(const char *path) { return LittleFS.remove(path); }
  bool remove(const String &path) { return LittleFS.remove(path); }
  bool rename(const char *from, const char *to) { return LittleFS.rename(from, to); }
  bool rename(const String &from, const String &to) { return LittleFS.rename(from, to); }

  uint8_t cardType() const { return CARD_MMC; }
  uint64_t cardSize() const { return LittleFS.totalBytes(); }

 private:
  bool mounted_ = false;
};

static InternalSdMmcShim SD_MMC;

#else

#include_next <SD_MMC.h>

#endif
