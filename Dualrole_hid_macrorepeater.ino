/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This example demonstrates use of both device and host, where
 * - Device run on native usb controller (roothub port0)
 * - Host depending on MCUs run on either:
 *   - rp2040: bit-banging 2 GPIOs with the help of Pico-PIO-USB library (roothub port1)
 *
 * Requirements:
 * - For rp2040:
 *   - [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library
 *   - 2 consecutive GPIOs: D+ is defined by PIN_USB_HOST_DP, D- = D+ +1
 *   - Provide VBus (5v) and GND for peripheral
 *   - CPU Speed must be either 120 or 240 Mhz. Selected via "Menu -> CPU Speed"
 */

#define PRINT_SERIAL_DELAY 2000  // milliseconds

#include "pin_setup_helper.h"
#include <SimRacingController.h>

// USBHost is defined in usbh_helper.h
#include "usbh_helper.h"

// neopixel must be included AFTER "pio_usb.h", in order to invoke pio2
#include <Adafruit_NeoPixel.h>

// #include "usbh_extension.ino"  // inclusion is automatic


//------------- TinyUSB HID Globals -------------//

// Report ID
enum {
  RID_KEYBOARD = 1,
  RID_MOUSE = 2,
  RID_CONSUMER_CONTROL = 3,  // Media, volume etc ...
};

// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL))
};

// valid values can be found in enum{} on line 897
// of Adafruit_TinyUSB_Library\src\class\hid\usages.h
static uint16_t consumer_keys_list[6] = {
  HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK,
  HID_USAGE_CONSUMER_PLAY_PAUSE,
  HID_USAGE_CONSUMER_SCAN_NEXT_TRACK,
  HID_USAGE_CONSUMER_MUTE,
  HID_USAGE_CONSUMER_VOLUME_DECREMENT,
  HID_USAGE_CONSUMER_VOLUME_INCREMENT,
};

// USB HID object
Adafruit_USBD_HID usb_hid;


//------------- Macro Globals -------------//
// initialize as if all switches are open
static bool is_recording = false;
static bool is_playing = false;
static bool is_passthrough_on = true;  // passthrough by default
static bool is_delay_on = false;

#define MACRO_BUFFER_SIZE 1000
static hid_keyboard_report_t macroBuffer[MACRO_BUFFER_SIZE];
static unsigned long delayBuffer[MACRO_BUFFER_SIZE];
// unsigned long *macro_starttime = &delayBuffer[0];
static size_t macro_len = 0;
static size_t old_macro_len = 0;


//------------- Macro Implementation -------------//
static const hid_keyboard_report_t null_report = { 0, 0, 0, 0, 0, 0, 0, 0 };

void usb_hid_send_my_rid_keyboard_report(const hid_keyboard_report_t this_report[]) {
  while (!usb_hid.ready()) { yield(); }
  usb_hid.sendReport(RID_KEYBOARD, this_report, sizeof(hid_keyboard_report_t));
}

// Macro playback: Send all recorded reports to PC in order
void play_macro() {
  usb_hid_send_my_rid_keyboard_report(&macroBuffer[0]);
  if (should_delay()) {
    for (size_t i = 1; i < macro_len; ++i) {
      delay(delayBuffer[i]);
      usb_hid_send_my_rid_keyboard_report(&macroBuffer[i]);
    }
  } else {
    for (size_t i = 1; i < macro_len; ++i) {
      delay(15);  // 66hz, 400wpm
      usb_hid_send_my_rid_keyboard_report(&macroBuffer[i]);
    }
  }
  usb_hid_send_my_rid_keyboard_report(&null_report);
}

void clear_macro() {
  old_macro_len = macro_len;
  macro_len = 0;
  Serial.println("Macro cleared.");
}

void undo_clear_macro() {
  macro_len = old_macro_len;
  // old_macro_len = 0;
  Serial.printf("Macro reset to %u.\r\n", macro_len);
}

// Save the received keyboard report to macro buffer
void save_to_macro(const hid_keyboard_report_t report[]) {
  unsigned long now = millis();
  if (macro_len == 0) {
    macroBuffer[0] = *report;
    delayBuffer[0] = now;
    macro_len = 1;
    Serial.printf("Macro started, macro starttime: %u\r\n", delayBuffer[0]);
  } else if (macro_len < MACRO_BUFFER_SIZE) {
    macroBuffer[macro_len] = *report;
    delayBuffer[macro_len] = now - delayBuffer[0];
    delayBuffer[0] = now;
    macro_len++;
    Serial.printf("Macro step %d saved.\r\n", macro_len);
  } else {
    Serial.println("Macro buffer full, cannot save more steps.");
  }
}


//------------- Switch Logic -------------//
bool should_save_to_macro() {
  return is_recording && !is_playing;
}

bool should_send_passthrough() {
  return !is_playing && (is_passthrough_on || !is_recording);
}

bool should_delay() {
  return is_delay_on;
}


//------------- Switch Implementation -------------//
const int MATRIX_ROWS = 2;
const int MATRIX_COLS = 5;
const int rowPins[MATRIX_ROWS] = {PIN_KEYPAD_R1, PIN_KEYPAD_R2};
const int colPins[MATRIX_COLS] = {PIN_KEYPAD_C1, PIN_KEYPAD_C2, PIN_KEYPAD_C3, PIN_KEYPAD_C4, PIN_KEYPAD_C5};

SimRacingController switchBoard;

void onMatrixChange(int profile, int row, int col, bool is_pressed) {
  // row 0 col 0 play macro
  if (row == 0 && col == 0) {
    if (is_pressed) {
      is_playing = true;
      Serial.println("Macro playback started.");
      play_macro();
      is_playing = false;
      Serial.println("Macro playback finished.");
    }
  }
  // row 1 col 0 start/stop recording
  else if (row == 1 && col == 0) {
    if (is_pressed) {
      if (is_recording) {
        is_recording = false;
        Serial.println("Macro recording stopped.");
        if (macro_len == 0) {
          Serial.println("No macro recorded.");
          undo_clear_macro();
        }
      } else {
        is_recording = true;
        Serial.println("Macro recording started.");
        if (macro_len > 0) {
          Serial.printf("Overwriting existing macro of length %u.\r\n", macro_len);
          clear_macro();
        }
      }
    }
  }
}


//------------- Lights Globals -------------//
// from "Adafruit Neopixel"/strandtest_nodelay.ino
unsigned long pixelPrevious = 0;        // Previous Pixel Millis
// unsigned long patternPrevious = 0;      // Previous Pattern Millis
// int           patternCurrent = 0;       // Current Pattern Number
// int           patternInterval = 5000;   // Pattern Interval (ms)
// bool          patternComplete = false;

int           pixelInterval = 50;       // Pixel Interval (ms)
// int           pixelQueue = 0;           // Pattern Pixel Queue
int           pixelCycle = 0;           // Pattern Pixel Cycle
uint16_t      pixelNumber = LED_COUNT;  // Total Number of Pixels
int           pixelCycleInterval = 256/pixelNumber;           // Color Interval

Adafruit_NeoPixel strip(LED_COUNT, PIN_KEYPAD_NEOPIXEL, NEO_GRB + NEO_KHZ800);


//------------- Lights Implementation -------------//
void neoPixel_init() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void neoPixel_update(unsigned long currentMillis) {
  // unsigned long currentMillis = millis();                     //  Update current time

  if(currentMillis - pixelPrevious >= pixelInterval) {        //  Check for expired time
    pixelPrevious = currentMillis;                            //  Run current frame
    rainbow(); // Flowing rainbow cycle along the whole strip
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Rainbow cycle along whole strip.
void rainbow() {
  for(uint16_t i=0; i < pixelNumber; i++) {
    strip.setPixelColor(i, Wheel((i*pixelCycleInterval + pixelCycle) & 255)); //  Update delay time
  }
  strip.show();                             //  Update strip to match
  pixelCycle++;                             //  Advance current cycle
  if(pixelCycle >= 256)
    pixelCycle = 0;                         //  Loop the cycle back to the begining
}


//--------------------------------------------------------------------+
// For RP2040 use both core0 for device stack, core1 for host stack
//--------------------------------------------------------------------+


//------------- Core0 -------------//
void setup() {
  Serial.begin(115200);

#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  // wait for native usb
  for (int i = 0; i < PRINT_SERIAL_DELAY; i += 10) {
    if (Serial) { break; }
    delay(10);
  }
#endif

  usb_hid.setPollInterval(2);  // milliseconds
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("TinyUSB HID Composite\r\n");
  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  // wait for native usb again
  for (int i = 0; i < PRINT_SERIAL_DELAY; i += 10) {
    if (Serial) { break; }
    delay(10);
  }
#endif

  switchBoard.setMatrix(rowPins, MATRIX_ROWS, colPins, MATRIX_COLS);
  // switchBoard.setGpio(gpioPins, 8);
  // switchBoard.setEncoders(encoderPinsA, encoderPinsB, encoderBtnPins, NUM_ENCODERS);
  switchBoard.setProfiles(1);
  switchBoard.setDebounceTime(50, 5);  // matrix/gpio=50ms, encoder=5ms

  // Set callbacks
  switchBoard.setMatrixCallback(onMatrixChange);
  // switchBoard.setGpioCallback(onGpioChange);
  // switchBoard.setEncoderCallback(onEncoderChange);
  // switchBoard.setEncoderButtonCallback(onEncoderButtonChange);

  switchBoard.begin();

  Serial.println("TinyUSB Macro Recorder Example");
}

void loop() {
  switchBoard.update();
}


//------------- Core1 -------------//
void setup1() {
#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  // wait for native usb
  for (int i = 0; i < PRINT_SERIAL_DELAY; i += 10) {
    if (Serial) { break; }
    delay(10);
  }
#endif
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  tuh_init_consumer_settings(consumer_keys_list, 6);

  neoPixel_init();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

void loop1() {
  USBHost.task();
  neoPixel_update(millis());
  Serial.flush();
}


//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {

  // Invoked when device with hid interface is mounted
  // Report descriptor is also available for use.
  // tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
  // descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
  // it will be skipped therefore report_desc = NULL, desc_len = 0
  void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    Serial.printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
    Serial.printf("VID = %04x, PID = %04x\r\n", vid, pid);

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
      Serial.printf("HID Keyboard\r\n");
      if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
      }
      return;
    }

    tuh_hid_report_info_t info;
    uint16_t consumer_page_start, consumer_page_end;
    bool found = tuh_hid_get_consumer_page(
      &info, &consumer_page_start, &consumer_page_end,
      desc_report, desc_len);
    if (!found) {
      // Serial.printf("this instance is not a keyboard protocol");
      return;
    }

    Serial.printf("HID Consumer Control\r\n");
    bool success = tuh_compute_consumer_page_values(
      desc_report, consumer_page_start, consumer_page_end,
      instance, info.report_id);
    if (!success) {
      Serial.printf("Error: cannot compute report_size, instance, report_id, \r\n");
      return;
    }

    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.printf("Error: cannot request to receive report\r\n");
    }
    return;
  }

  // Invoked when device with hid interface is un-mounted
  void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    if (instance == get_tuh_consumer_instance()) {
      tuh_init_consumer_settings();
    }
    clear_macro();
  }

  // Invoked when received report from device via interrupt endpoint
  void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (instance == 0) {  // boot keyboard
                          // Serial.printf("Received report from instance %d, len = %u\r\n", instance, len);
      if (len != 8) {
        Serial.printf("report len = %u NOT 8, probably something wrong !!\r\n", len);
      } else {
        const hid_keyboard_report_t* boot_report = (const hid_keyboard_report_t*)report;
        if (should_save_to_macro()) {
          save_to_macro(boot_report);
        }

        if (should_send_passthrough()) {
          usb_hid_send_my_rid_keyboard_report(boot_report);
        }
      }

    } else if (instance == get_tuh_consumer_instance()) {
      // Serial.printf("Received report from consumer control instance %d, len = %u\r\n", instance, len);
      uint16_t report_consumer_key = tuh_process_consumer_report(report, len);

      // NOTE: for better performance you should save/queue remapped report instead of
      // blocking wait for usb_hid ready here
      while (!usb_hid.ready()) { yield(); }
      usb_hid.sendReport(RID_CONSUMER_CONTROL, &report_consumer_key, sizeof(report_consumer_key));
    } else {
      Serial.printf("Received report from unknown instance %d, len = %u\r\n", instance, len);
    }

    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.printf("Error: cannot request to receive report\r\n");
    }
  }
}
