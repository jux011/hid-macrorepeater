
#include <LittleFS.h>

static bool FS_ENABLED = true;

enum MACRO_i {
  MACRO_OFF = 0,
  MACRO0 = 1,
  MACRO1 = 2,
  MACRO2 = 3,
  MACRO3 = 4,
};


void load_macro_from_FS(hid_keyboard_report_t out_macroBuffer[], unsigned long out_delayBuffer[], size_t* macro_len, int macro_index) {
  if (!FS_ENABLED) {
    return;
  }
  File f;
  switch (macro_index) {
    case MACRO0:
      f = LittleFS.open("/macro0", "r");
      break;
    case MACRO1:
      f = LittleFS.open("/macro1", "r");
      break;
    case MACRO2:
      f = LittleFS.open("/macro2", "r");
      break;
    case MACRO3:
      f = LittleFS.open("/macro3", "r");
      break;
    case MACRO_OFF:
      return;
  }
  if (f) {
    char buffer[12];
    *macro_len = 0;
    for (size_t i = 0; i < MACRO_BUFFER_SIZE; i++) {
      if (!f.available()) {
        break;
      }
      // read 12 bytes into buffer
      int bytes_read = f.readBytes(buffer, 12);
      if (bytes_read < 12) {
        break;
      }
      // copy the delay value from the buffer
      memcpy(&out_delayBuffer[i], buffer, 4);
      // copy the keycodes from the buffer
      memcpy(&out_macroBuffer[i], buffer + 4, 8);
      *macro_len = i + 1;
    }
    f.close();
  }
}


void write_macro_to_FS(const hid_keyboard_report_t out_macroBuffer[], const unsigned long out_delayBuffer[], size_t macro_len, int macro_index) {
  if (!FS_ENABLED) {
    return;
  }
  File f;
  switch (macro_index) {
    case MACRO0:
      f = LittleFS.open("/macro0", "w");
      break;
    case MACRO1:
      f = LittleFS.open("/macro1", "w");
      break;
    case MACRO2:
      f = LittleFS.open("/macro2", "w");
      break;
    case MACRO3:
      f = LittleFS.open("/macro3", "w");
      break;
    case MACRO_OFF:
      return;
  }
  if (f) {
    char buffer[12];
    for (size_t i = 0; i < macro_len; i++) {
      // copy the delay value into the buffer
      memcpy(buffer, &out_delayBuffer[i], 4);
      // copy the keycodes into the buffer
      memcpy(buffer + 4, &out_macroBuffer[i], 8);
      f.write((uint8_t *)buffer, 12);
    }
    f.close();
  }
}


void init_FS() {
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    FS_ENABLED = false;
    return;
  }

  FSInfo info;
  LittleFS.info(info);
  
  // 256 KB = 262144 bytes
  if (info.totalBytes < (256 * 1024)) {
      Serial.printf("CRITICAL ERROR: Wrong FS Size! Expected >256KB, got %d KB\n", info.totalBytes / 1024);
      FS_ENABLED = false;
      return;
  }
  return;
}