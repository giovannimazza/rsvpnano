#include "storage/SeedBooks.h"

#include <SD_MMC.h>
#include <cerrno>
#include <cstring>

#include "board/BoardConfig.h"
#include "storage/fs/StorageFiles.h"
#include "storage/fs/StoragePaths.h"

namespace {

constexpr const char *kSeedMarkerPath = "/config/.seed-books-v1";

constexpr const char kEuropeanLetterDemoBook[] = R"(@rsvp 1
@title European Letter Demo
@author Codex
@source docs/demo-books/european-letter-demo.rsvp
@chapter What This Shows
@para
This demo book exercises the accented, Baltic, Sami, and extended-Latin letters that the current firmware should now preserve, pace, and render correctly.
@para
If the update worked, these chapters should show real letters instead of question marks.

@chapter Quotes And Brackets
@para
“Quoted text,” ‘single quotes,’ «guillemets,» and ‹angle quotes› should stay visible.
@para
(Parentheses) [brackets] {braces} <angles> should stay wrapped around words.
@para
Fullwidth forms like （these）, ［these］, ｛these｝, and ＜these＞ should collapse into readable ASCII wrappers.

@chapter Germanic And Nordic
@para
Ärger größer Übermut Straße Fußgänger Mädchen schön kühl.
@para
Æsir Øresund Ålesund blåbær smørrebrød Þingvellir þjóðvegur veður.
@para
Õun öö ääni jääkülm.

@chapter French And Western European
@para
Œuvre cœur sœur garçon français Noël déjà façade maïs.
@para
¿Qué tal? ¡Aquí está! Señor, año, niño, canción, corazón, pingüino, vergüenza, acción y café should keep Spanish punctuation and accents readable.
@para
Português também deve funcionar: ação, coração, amanhã, informação, maçã, São Paulo, bênção e português.

@chapter Polish Romanian And Turkish
@para
Łódź książę Śląsk Żubr Źródło Ćma dźwięk pięć.
@para
Țară Știință înțelegere ăsta română înger.
@para
İzmir ışık çağdaş öğüt şeker üzüm.

@chapter Baltic
@para
Ābele Čiekurs Ēna Ģimene Īlens Ķirbis Ļaudis Ņem Šūpoles Ūdens Žagata.
@para
Ąžuolas Ėriukas Įžanga Šaltis Ųkis Ūkas Žingsnis.
@para
Pair bank Ą ą Ę ę Ė ė Į į Š š Ų ų Ū ū Ž ž.

@chapter Sami
@para
Áigi Čáhci Đálki Ŋuovža Šávza Ŧuorri Žárga.
@para
Pair bank Á á Č č Đ đ Ŋ ŋ Š š Ŧ ŧ Ž ž.

@chapter Full Pair Bank
@para
Æ æ Œ œ Ø ø Å å Ä ä Ö ö Ü ü Þ þ Ð ð Ł ł Č č Š š Ž ž Ă ă Ș ș Ț ț Ğ ğ Ş ş İ ı Ą ą Ę ę Ć ć Ń ń Ś ś Ź ź Ż ż Ā ā Ē ē Ģ ģ Ī ī Ķ ķ Ļ ļ Ņ ņ Ė ė Į į Ų ų Ū ū Đ đ Ŋ ŋ Ŧ ŧ.
)";

constexpr const char kGettingStartedBook[] = R"(@rsvp 1
@title Getting Started
@author RSVP Nano
@source built-in seed
@chapter Open This Book
@para
This book is bundled into flash so it appears in Books right after you flash the firmware.
@para
Add more seed files here to preload your own library.

@chapter Next Step
@para
You can replace this demo with your own .rsvp or .txt content later without changing the firmware UI.
)";

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

}  // namespace

bool seedDemoBooksIfNeeded() {
#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
  if (StorageFiles::fileExists(kSeedMarkerPath)) {
    return false;
  }

  if (!StorageFiles::ensureDirectory(StoragePaths::kBooksPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kBookFilesPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kArticleFilesPath, "seed-books") ||
      !StorageFiles::ensureDirectory(StoragePaths::kConfigPath, "seed-books")) {
    return false;
  }

  const bool demoOk = writeTextFile("/books/books/european-letter-demo.rsvp", kEuropeanLetterDemoBook);
  const bool starterOk = writeTextFile("/books/books/getting-started.rsvp", kGettingStartedBook);
  if (!demoOk || !starterOk) {
    return false;
  }

  if (!writeTextFile(kSeedMarkerPath, "seed-books-v1\n")) {
    return false;
  }

  Serial.println("[seed-books] built-in books imported");
  return true;
#else
  return false;
#endif
}
