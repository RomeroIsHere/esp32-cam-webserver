#include "FS.h"
#include "SD_MMC.h"

#define FORMAT_SD_IF_FAILED true
#define PREFERENCES_MAX_SIZE 500

#define PREFERENCES_FILE "/esp32cam-preferences.json"
#define FACE_DB_FILE  "/esp32cam-facedb.bin"

extern void dumpPrefs(fs::FS &fs);
extern void loadPrefs(fs::FS &fs);
extern void removePrefs(fs::FS &fs);
extern void savePrefs(fs::FS &fs);
extern void loadFaceDB(fs::FS &fs);
extern void removeFaceDB(fs::FS &fs);
extern void saveFaceDB(fs::FS &fs);

extern void filesystemStart();
