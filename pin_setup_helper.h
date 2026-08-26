// waveshare rp2040 Zero  +  2x5 ameoba_king_pcb
static_assert(ARDUINO_ARCH_RP2040);

// diodes are setup 29 14 high -> 15 27 13 12 28 low
// roncoa calls digitalWrite(rowPins, HIGH) and bool(digitalRead(colPins) == LOW
// therefore, swap rows and columns in software

#define PIN_KEYPAD_C1 15
#define PIN_KEYPAD_C2 27
#define PIN_KEYPAD_C3 13
#define PIN_KEYPAD_C4 12
#define PIN_KEYPAD_C5 28

#define PIN_KEYPAD_R1 29
#define PIN_KEYPAD_R2 14

#define PIN_KEYPAD_NEOPIXEL 26

#define LED_COUNT 10

#define PIN_USB_HOST_DP 6
// #define PIN_USB_HOST_DM 7 // implicit
