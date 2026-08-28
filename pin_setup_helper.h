// waveshare pizero with gamepi hat
#ifndef ARDUINO_ARCH_RP2040
  #error "Error: ARDUINO_ARCH_RP2040 is not defined! target hardware is RP2040."
#endif

#define PIN_BUTTON1_IN 21
#define PIN_BUTTON2_IN 20
#define PIN_BUTTON3_IN 16

#define PIN_JOYSTICK_UP_IN 9 // no longer 6
#define PIN_JOYSTICK_DOWN_IN 19
#define PIN_JOYSTICK_LEFT_IN 15
#define PIN_JOYSTICK_RIGHT_IN 26
#define PIN_JOYSTICK_PRESS_IN 13

#define PIN_USB_HOST_DP 6
// #define PIN_USB_HOST_DM 7 // implicit

static void set_pinMode() {
  // SimRacingController configures buttons as  open INPUT_PULLUP, closed LOW
  // this matches the hardware exactly
  // no need to manually set pinMode or digitalWrite HIGH
  // pinMode(PIN_BUTTON1_IN, INPUT_PULLUP);
  // pinMode(PIN_BUTTON2_IN, INPUT_PULLUP);
  // pinMode(PIN_BUTTON3_IN, INPUT_PULLUP);

  // pinMode(PIN_JOYSTICK_UP_IN, INPUT_PULLUP);
  // pinMode(PIN_JOYSTICK_DOWN_IN, INPUT_PULLUP);
  // pinMode(PIN_JOYSTICK_LEFT_IN, INPUT_PULLUP);
  // pinMode(PIN_JOYSTICK_RIGHT_IN, INPUT_PULLUP);
  // pinMode(PIN_JOYSTICK_PRESS_IN, INPUT_PULLUP);
}

static void set_output_poweron() {
  // all buttons are open INPUT_PULLUP, closed LOW
  // nothing to poweron
}

// set_pinMode();
// set_output_poweron();
