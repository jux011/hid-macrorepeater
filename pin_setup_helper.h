
static_assert(ARDUINO_ARCH_RP2040);

// TODO: redesignate pins
#define PIN_BUTTON_OUT 10
#define PIN_BUTTON_IN 11
#define PIN_SWITCH1_LED 29
// #define PIN_SWITCH1_OUT 999
#define PIN_SWITCH1_IN 28
#define PIN_SWITCH2_OUT 27
#define PIN_SWITCH2_IN 26
#define PIN_SWITCH3_OUT 15
#define PIN_SWITCH3_IN 14
#define PIN_USB_HOST_DP 6
// #define PIN_USB_HOST_DM 7 // implicit

static void set_pinMode() {
  pinMode(PIN_BUTTON_IN, INPUT_PULLDOWN);
  pinMode(PIN_BUTTON_OUT, OUTPUT);

  pinMode(PIN_SWITCH1_LED, OUTPUT);
  digitalWrite(PIN_SWITCH1_LED, LOW);

  pinMode(PIN_SWITCH1_IN, INPUT_PULLDOWN);
  // pinMode(PIN_SWITCH1_OUT, OUTPUT); // Switch 1 is wired to 3.3V directly

  pinMode(PIN_SWITCH2_IN, INPUT_PULLDOWN);
  pinMode(PIN_SWITCH2_OUT, OUTPUT);

  pinMode(PIN_SWITCH3_IN, INPUT_PULLDOWN);
  pinMode(PIN_SWITCH3_OUT, OUTPUT);
}

static void set_output_poweron() {
  digitalWrite(PIN_BUTTON_OUT, HIGH);
  // digitalWrite(PIN_SWITCH1_OUT, HIGH); // Switch 1 is wired to 3.3V directly
  digitalWrite(PIN_SWITCH2_OUT, HIGH);
  digitalWrite(PIN_SWITCH3_OUT, HIGH);
}

// set_pinMode();
// set_output_poweron();
