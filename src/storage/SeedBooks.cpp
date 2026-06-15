#include "storage/SeedBooks.h"

#include <SD_MMC.h>

#include <cerrno>
#include <cstring>
#include <vector>

#include "storage/SeedBooks.generated.h"
#include "storage/fs/StorageFiles.h"
#include "storage/fs/StoragePaths.h"

namespace {

constexpr const char *kSeedMarkerPath = "/config/.seed-books-v1";

bool writeTextFile(const char *path, const char *contents) {
  errno = 0;
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("[seed-books] open failed: %s errno=%d\n", path, errno);
    return false;
  }

  const size_t expected = strlen(contents);
  const size_t written = file.print(contents);
  file.close();
  if (written != expected) {
    Serial.printf("[seed-books] short write: %s wrote=%u expected=%u\n", path,
                  static_cast<unsigned int>(written), static_cast<unsigned int>(expected));
    return false;
  }

  return true;
}

bool readMarker(String *markerContents) {
  File file = SD_MMC.open(kSeedMarkerPath, FILE_READ);
  if (!file) {
    return false;
  }

  *markerContents = file.readString();
  file.close();
  return true;
}

std::vector<String> parseMarkedPaths(const String &markerContents) {
  std::vector<String> paths;
  int start = 0;
  bool firstLine = true;

  while (start <= markerContents.length()) {
    int end = markerContents.indexOf('\n', start);
    if (end < 0) {
      end = static_cast<int>(markerContents.length());
    }

    String line = markerContents.substring(start, end);
    line.trim();
    if (!firstLine && !line.isEmpty()) {
      paths.push_back(line);
    }

    if (end >= static_cast<int>(markerContents.length())) {
      break;
    }

    firstLine = false;
    start = end + 1;
  }

  return paths;
}

bool removePathIfExists(const String &path) {
  if (!StorageFiles::fileExists(path)) {
    return true;
  }
  if (!SD_MMC.remove(path)) {
    Serial.printf("[seed-books] remove failed: %s\n", path.c_str());
    return false;
  }
  return true;
}

bool removePreviousSeededFiles(const String &markerContents) {
  for (const String &path : parseMarkedPaths(markerContents)) {
    if (!removePathIfExists(path)) {
      return false;
    }
  }
  return true;
}

bool writeMarker() {
  File file = SD_MMC.open(kSeedMarkerPath, FILE_WRITE);
  if (!file) {
    Serial.printf("[seed-books] open marker failed: %s\n", kSeedMarkerPath);
    return false;
  }

  file.println(kSeedManifestHash);
  for (size_t i = 0; i < kSeedBookCount; ++i) {
    file.println(kSeededBooks[i].path);
  }
  file.close();
  return true;
}

}  // namespace

bool seedDemoBooksIfNeeded() {
#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  String markerContents;
  const bool hasMarker = readMarker(&markerContents);
  if (hasMarker) {
    const int newlineIndex = markerContents.indexOf('\n');
    const String markerHash = newlineIndex >= 0 ? markerContents.substring(0, newlineIndex) : markerContents;
    if (markerHash == kSeedManifestHash) {
      return false;
    }
  }

  if (!StorageFiles::ensureDirectory(StoragePaths::kBooksPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kBookFilesPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kArticleFilesPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kConfigPath, "seed-books")) {
    return false;
  }

  if (hasMarker && !removePreviousSeededFiles(markerContents)) {
    return false;
  }

  for (size_t i = 0; i < kSeedBookCount; ++i) {
    if (!writeTextFile(kSeededBooks[i].path, kSeededBooks[i].contents)) {
      return false;
    }
  }

  if (!writeMarker()) {
    return false;
  }

  Serial.printf("[seed-books] imported %u built-in books\n",
                static_cast<unsigned int>(kSeedBookCount));
  return true;
#else
  return false;
#endif
}
