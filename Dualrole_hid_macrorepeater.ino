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
size_t macro_len = 0;
size_t old_macro_len = 0;
bool macro_is_playing = false;
bool macro_is_recording = false;

// -------- Input Setup --------
int led_state = LOW;
const unsigned long debounce_delay = 50;
const int PIN_IN[4] = {PIN_BUTTON_IN, PIN_SWITCH1_IN, PIN_SWITCH2_IN, PIN_SWITCH3_IN};
unsigned long lastDebounceTimes[4] = {0,0,0,0};
bool lastSwitchStates[4] = {LOW, LOW, LOW, LOW};
bool switchStates[4] = {LOW, LOW, LOW, LOW};
bool sent_message[4] = {false, false, false, false}; // verbose input state reporting

// -------- Switch logic --------
bool should_save_to_macro(bool* booleanStates) {
  return booleanStates[1]; //== HIGH;
}

bool should_send_passthrough(bool* booleanStates) {
  return booleanStates[2] == LOW || booleanStates[1] == LOW;
}

bool should_power_led_on(bool* booleanStates) {
  return booleanStates[1] == HIGH;
}

//------------- Core0 -------------//
void setup() {
  Serial.begin(115200);
  set_pinMode();
  set_output_poweron();
  usb_hid.begin();

#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  while ( !Serial ) delay(10);   // wait for native usb
  Serial.print("TinyUSB Macro Recorder Example\r\n");
#endif
}

void loop() {
  const unsigned long now_timestamp = millis();
  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(PIN_IN[i]);
    if (reading != lastSwitchStates[i]) {
      lastDebounceTimes[i] = now_timestamp;
      lastSwitchStates[i] = reading;
    }

    if (((millis() - lastDebounceTimes[i]) > debounce_delay)
        && (reading != switchStates[i])) {
      switchStates[i] = reading;
    }
  }

  for (int i = 0; i < 4; i++) {  // verbose input state reporting
    if (switchStates[i] == HIGH) {
      if (!sent_message[i]) {
        Serial.printf("switch %u on\r\n", i);
        sent_message[i] = true;
      }
    } else {
      sent_message[i] = false;
    }
  }

  if (switchStates[0] == HIGH) {
    // if (macro_is_playing) {
    //   Serial.println("Button held down");
    // }
    if (!macro_is_playing) {
      macro_is_playing = true;
      if (macro_len > 0) {
        Serial.println("Button pressed, playing macro.");
        play_macro();
        // if ()
      } else {
        Serial.println("Button pressed, but macro is empty.");
      }
    }
  } else {
    macro_is_playing = false;
  }

  if (switchStates[1] == HIGH) {
    if (!macro_is_recording) {
      macro_is_recording = true;
      clear_macro();
    }
  } else {
    if (macro_is_recording) {
      macro_is_recording = false;
      if (macro_len <= 0) {
        undo_clear_macro();
      }
    }
  }

  if (should_power_led_on(switchStates) && !led_state) {
    led_state = HIGH;
    set_led_blinking(600,50.0);    
  }
  else if (!should_power_led_on(switchStates) && led_state) {
    led_state = LOW;
    set_led_off();
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
  Serial.flush();
}

//------------- Macro -------------//
// Macro playback: Send all recorded reports to PC in order
void play_macro() {
  for (size_t i = 0; i < macro_len; ++i) {
    while (!usb_hid.ready()) {
      yield();
    }
    usb_hid.sendReport(0, &macroBuffer[i], sizeof(hid_keyboard_report_t));
    delay(15);  // 66hz, 400wpm
  }
  const uint8_t null_report[8] = { 0 };
  usb_hid.sendReport(0, null_report, sizeof(hid_keyboard_report_t));
  Serial.println("Macro playback finished.");
}

void clear_macro() {
  old_macro_len = macro_len;
  macro_len = 0;
  Serial.println("Macro cleared.");
}

void undo_clear_macro() {
  macro_len = old_macro_len;
  // old_macro_len = 0;
  Serial.printf("Macro reset to %u.\r\n",macro_len);
}

// Save the received keyboard report to macro buffer
void save_to_macro(const hid_keyboard_report_t *report) {
  if (macro_len < MACRO_BUFFER_SIZE) {
    macroBuffer[macro_len++] = *report;
    Serial.printf("Macro step %d saved.\r\n", macro_len);
  } else {
    Serial.println("Macro buffer full, cannot save more steps.");
  }
}

// -------------------------------------------------------------------------
// TinyUSB Host callbacks
// -------------------------------------------------------------------------
extern "C" {

  // --- When HID device (like keyboard) is mounted ---
  void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
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
    }
  }

  // --- When HID device is unmounted ---
  void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    clear_macro();
  }

  // --- When HID report is received (save as macro step) ---
  void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (len != 8) {
      Serial.printf("report len = %u NOT 8, probably something wrong !!\r\n", len);
    } else {
      if (should_save_to_macro(switchStates)) {
        save_to_macro((hid_keyboard_report_t const *)report);
      }

      if (should_send_passthrough(switchStates)) {
        while (!usb_hid.ready()) { yield(); }
        usb_hid.sendReport(0, report, sizeof(hid_keyboard_report_t));
      }
    }

    // Continue to request the next report as before...
    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.printf("Error: cannot request to receive report\r\n");
    }
  }

}  // extern "C"
