#include "esp_camera.h"
#include "src/jsonlib/jsonlib.h"
#include "storage.h"

// These are defined in the main .ino file
extern void flashLED(int flashtime);
extern int myRotation;              // Rotation
extern int lampVal;                 // The current Lamp value
extern int autoLamp;                // Automatic lamp mode
extern int8_t detection_enabled;    // Face detection enable
extern int8_t recognition_enabled;  // Face recognition enable
//This One Comes fomr app_httpd.cpp


#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

extern int reenrollFace(dl_matrix3du_t *aligned_face);


static char FileNameBuffer[MAX_FILENAME_LENGTH];

/*
 * Useful utility when debugging... 
 */

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  digitalWrite(4,LOW);
  Serial.printf("Listing File System directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void dumpPrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    // Dump contents for debug
    File file = fs.open(PREFERENCES_FILE, FILE_READ);
    int countSize = 0;
    while (file.available() && countSize <= PREFERENCES_MAX_SIZE) {
        Serial.print(char(file.read()));
        countSize++;
    }
    Serial.println("");
    file.close();
  } else {
    Serial.printf("%s not found, nothing to dump.\r\n", PREFERENCES_FILE);
  }
}

void loadPrefs(fs::FS &fs){
  digitalWrite(4,LOW);
  if (fs.exists(PREFERENCES_FILE)) {
    // read file into a string
    String prefs;
    Serial.printf("Loading preferences from file %s\r\n", PREFERENCES_FILE);
    File file = fs.open(PREFERENCES_FILE, FILE_READ);
    if (!file) {
      Serial.println("Failed to open preferences file for reading, maybe corrupt, removing");
      removePrefs(fs);
      return;
    }
    size_t size = file.size();
    if (size > PREFERENCES_MAX_SIZE) {
      Serial.println("Preferences file size is too large, maybe corrupt, removing");
      removePrefs(fs);
      return;
    }
    while (file.available()) {
        prefs += char(file.read());
        if (prefs.length() > size) {
          // corrupted files can return data beyond their declared size.
          Serial.println("Preferences file failed to load properly, appears to be corrupt, removing");
          removePrefs(fs);
          return;
        }
    }
    // get sensor reference
    sensor_t * s = esp_camera_sensor_get();
    // process all the settings
    lampVal = jsonExtract(prefs, "lamp").toInt();
    autoLamp = jsonExtract(prefs, "autolamp").toInt();
    s->set_framesize(s, (framesize_t)jsonExtract(prefs, "framesize").toInt());
    s->set_quality(s, jsonExtract(prefs, "quality").toInt());
    s->set_brightness(s, jsonExtract(prefs, "brightness").toInt());
    s->set_contrast(s, jsonExtract(prefs, "contrast").toInt());
    s->set_saturation(s, jsonExtract(prefs, "saturation").toInt());
    s->set_special_effect(s, jsonExtract(prefs, "special_effect").toInt());
    s->set_wb_mode(s, jsonExtract(prefs, "wb_mode").toInt());
    s->set_whitebal(s, jsonExtract(prefs, "awb").toInt());
    s->set_awb_gain(s, jsonExtract(prefs, "awb_gain").toInt());
    s->set_exposure_ctrl(s, jsonExtract(prefs, "aec").toInt());
    s->set_aec2(s, jsonExtract(prefs, "aec2").toInt());
    s->set_ae_level(s, jsonExtract(prefs, "ae_level").toInt());
    s->set_aec_value(s, jsonExtract(prefs, "aec_value").toInt());
    s->set_gain_ctrl(s, jsonExtract(prefs, "agc").toInt());
    s->set_agc_gain(s, jsonExtract(prefs, "agc_gain").toInt());
    s->set_gainceiling(s, (gainceiling_t)jsonExtract(prefs, "gainceiling").toInt());
    s->set_bpc(s, jsonExtract(prefs, "bpc").toInt());
    s->set_wpc(s, jsonExtract(prefs, "wpc").toInt());
    s->set_raw_gma(s, jsonExtract(prefs, "raw_gma").toInt());
    s->set_lenc(s, jsonExtract(prefs, "lenc").toInt());
    s->set_vflip(s, jsonExtract(prefs, "vflip").toInt());
    s->set_hmirror(s, jsonExtract(prefs, "hmirror").toInt());
    s->set_dcw(s, jsonExtract(prefs, "dcw").toInt());
    s->set_colorbar(s, jsonExtract(prefs, "colorbar").toInt());
    detection_enabled = jsonExtract(prefs, "face_detect").toInt();
    recognition_enabled = jsonExtract(prefs, "face_recognize").toInt();
    myRotation = jsonExtract(prefs, "rotate").toInt();
    // close the file
    file.close();
    dumpPrefs(fs);
  } else {
    Serial.printf("Preference file %s not found; using system defaults.\r\n", PREFERENCES_FILE);
  }
}

void savePrefs(fs::FS &fs){
  digitalWrite(4,LOW);
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Updating %s\r\n", PREFERENCES_FILE);
  } else {
    Serial.printf("Creating %s\r\n", PREFERENCES_FILE);
  }
  
  Serial.println("Preferences file Opening");

  File file = fs.open(PREFERENCES_FILE, FILE_WRITE);
  if (!file) {
      Serial.println("Failed to open preferences file for Writing, Exiting");
      return;
  }else{
      Serial.println("Opened preferences file for Writing");
  }
  static char json_response[1024];
  sensor_t * s = esp_camera_sensor_get();
  Serial.println("Got ESP CAM Reference");
  char * p = json_response;
  *p++ = '{';
  p+=sprintf(p, "\"lamp\":%i,", lampVal);
  p+=sprintf(p, "\"autolamp\":%u,", autoLamp);
  p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p+=sprintf(p, "\"quality\":%u,", s->status.quality);
  p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p+=sprintf(p, "\"awb\":%u,", s->status.awb);
  p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p+=sprintf(p, "\"aec\":%u,", s->status.aec);
  p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p+=sprintf(p, "\"agc\":%u,", s->status.agc);
  p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
  p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p+=sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
  p+=sprintf(p, "\"face_detect\":%u,", detection_enabled);
  p+=sprintf(p, "\"face_recognize\":%u,", recognition_enabled);
  p+=sprintf(p, "\"rotate\":\"%d\"", myRotation);
  *p++ = '}';
  *p++ = 0;
  Serial.println("finished Getting All Settings, Writing as Print");
  file.print(json_response);
  Serial.println("finished Saving All Settings, Closing File");
  file.close();
  Serial.println("Closed File");
  dumpPrefs(fs);
}

void removePrefs(fs::FS &fs) {
  digitalWrite(4,LOW);
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Removing %s\r\n", PREFERENCES_FILE);
    if (!fs.remove(PREFERENCES_FILE)) {
      Serial.println("Error removing preferences");
    }
  } else {
    Serial.println("No saved preferences file to remove");
  }
}

void strFaceDBNameBuilder(int faceID=1, int sample=0, const char *start=FACE_DB_FILE, const char *fileExt=FACE_DB_RGB888_FILE_EXT){
  sprintf(FileNameBuffer,"%s%d-%d%s",start,faceID,sample,fileExt);
}

void saveFaceDB(fs::FS &fs, dl_matrix3du_t *aligned_face, int faceID, int sample) {
  digitalWrite(4,LOW);
  strFaceDBNameBuilder(faceID,sample);
  //TODO: Fucking Fix this
  if (fs.exists(FileNameBuffer)) {
    Serial.printf("Updating %s\r\n", FileNameBuffer);
  } else {
    Serial.printf("Creating %s\r\n", FileNameBuffer);
  }
  File file = fs.open(FileNameBuffer, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(aligned_face->item, aligned_face->h *  aligned_face->w * 3); // payload (image), payload length
    Serial.printf("Saved file to path: %s\r\n", FileNameBuffer);
  }
  file.close();
  return;
}
void loadFaceDB(fs::FS &fs) {//FACE_WIDTH
  digitalWrite(4,LOW);
  //TOD:Go on a Loop to REload every image in Order
  if (fs.exists(FileNameBuffer)) {
    // read file into a string
    String prefs;
    Serial.printf("Loading Faces from file %s\r\n", FileNameBuffer);
    File file = fs.open(FileNameBuffer, FILE_READ);
    if (!file) {
      Serial.println("Failed to open faces file for reading, maybe corrupt, removing");
      removeFaceDB(fs);
      return;
    }
    // get sensor reference
    file.close();
  } else {
    Serial.printf("Face file %s not found; using system defaults.\r\n", FileNameBuffer);
  }
}
void removeFaceDB(fs::FS &fs) {
  digitalWrite(4,LOW);
  if (fs.exists(FileNameBuffer)) {
    Serial.printf("Removing %s\r\n", FileNameBuffer);
    if (!fs.remove(FileNameBuffer)) {
      Serial.println("Error removing preferences");
    }
  } else {
    Serial.printf("%s Does not Exist\r\n", FileNameBuffer);
  }
  return;
}

void filesystemStart(){
  digitalWrite(4,LOW);
  Serial.println("Attempting to Start SD Filesystem");
  while ( !SD_MMC.begin()) {
    // if we sit in this loop something is wrong; 
    // if no existing SD partition exists one should be automagically created.
    Serial.println("SD Mount failed, this can happen on first-run initialisation.");
    Serial.println("If it happens repeatedly check if a SD is present for your board?");
    for (int i=0; i<10; i++) {
      flashLED(100); // Show SD failure
      delay(100);
    }
    delay(1000);
    Serial.println("Retrying..");
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    for (int i=0; i<10; i++) {
      flashLED(200); // Show SD failure
      delay(100);
    }
    delay(500);
    Serial.println("Retrying..");
  }else{
    Serial.println("SD Card Detected");
    Serial.printf("SD Card of type:  %d\r\n", cardType);
  }
  
  Serial.println("Internal filesystem contents");
  listDir(SD_MMC, "/", 0);
}
