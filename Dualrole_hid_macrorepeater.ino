/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#define PRINT_SERIAL_DELAY 1

// USBHost is defined in usbh_helper.h
#include "pin_setup_helper.h"

#include "usbh_helper.h"

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// USB HID object: desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_KEYBOARD, 2, false);

// -------- Macro Buffer --------
#define MACRO_BUFFER_SIZE 250
hid_keyboard_report_t macroBuffer[MACRO_BUFFER_SIZE];
size_t macroLen = 0;
bool macro_is_playing = false;

// -------- Input Setup --------
const unsigned long debounceDelay = 50;
unsigned long lastDebounceTime = 0;
bool lastSwitchState = LOW;
bool switchState = LOW;

void setup() {
  Serial.begin(115200);
  set_pinMode();
  usb_hid.begin();

#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  while ( !Serial ) delay(10);   // wait for native usb
  Serial.print("TinyUSB Macro Recorder Example\r\n");
#endif
}

#if defined(ARDUINO_ARCH_RP2040)
//--------------------------------------------------------------------+
// For RP2040 use both core0 for device stack, core1 for host stack
//--------------------------------------------------------------------+

void loop() {
  bool reading = digitalRead(PIN_BUTTON_IN);
  if (reading != lastSwitchState) {
    lastDebounceTime = millis();
    lastSwitchState = reading;
  }
  
  if (((millis() - lastDebounceTime) > debounceDelay)
    && (reading != switchState)) {
    switchState = reading;
  }

  if (switchState == HIGH) {
    if (macro_is_playing) {
      Serial.println("Button held down");
    }
    else if (macroLen <= 0) {
      Serial.println("Button pressed, but macro is empty");
      macro_is_playing = true;
    } 
    else {
      Serial.println("Button pressed, playing macro");
      macro_is_playing = true;
      play_macro();
    }
  }
  else {
    macro_is_playing = false;
  }
}

//------------- Core1 -------------//
void setup1() {
#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  while ( !Serial ) delay(10);   // wait for native usb
#endif
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

void loop1() {
  USBHost.task();
}
#endif

// Macro playback: Send all recorded reports to PC in order
void play_macro() {
  for (size_t i = 0; i < macroLen; ++i) {
    while (!usb_hid.ready()) {
      yield();
    }
    usb_hid.sendReport(0, &macroBuffer[i], sizeof(hid_keyboard_report_t));
    delay(15); // 66hz, 400wpm
  }
  macroLen = 0; // Clear the macro buffer after playback
  Serial.println("Macro playback finished and buffer cleared.");
}

// Save the received keyboard report to macro buffer
void save_to_macro(const hid_keyboard_report_t *report) {
  if (macroLen < MACRO_BUFFER_SIZE) {
    macroBuffer[macroLen++] = *report;
    Serial.printf("Macro step %d saved.\r\n", macroLen);
  } else {
    Serial.println("Macro buffer full, cannot save more steps.");
  }
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C"
{

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  (void) desc_report;
  (void) desc_len;
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
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  macroLen = 0;
}

// --- When HID report is received (save as macro step) ---
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  if (len != 8) {
    Serial.printf("report len = %u NOT 8, probably something wrong !!\r\n", len);
  } else {
    save_to_macro((hid_keyboard_report_t const *)report);
    // Optional: print contents of the report here for debugging
    while (!usb_hid.ready()) {
      yield();
    }
    usb_hid.sendReport(0, report, sizeof(hid_keyboard_report_t));
  }

  // Continue to request the next report as before...
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    Serial.printf("Error: cannot request to receive report\r\n");
  }
}

}
