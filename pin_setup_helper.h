
static_assert(ARDUINO_ARCH_RP2040);

#define PIN_BUTTON_OUT 10
#define PIN_BUTTON_IN 11
#define PIN_SWITCH1_LED 29
// #define PIN_SWITCH1_OUT 999
#define PIN_SWITCH1_IN 28
#define PIN_SWITCH2_OUT 27
#define PIN_SWITCH2_IN 26
#define PIN_SWITCH3_OUT 15
#define PIN_SWITCH3_IN 14
#define PIN_USB_HOST_DP 12
// #define PIN_USB_HOST_DPMINUS 13 // implicit

#include "pwm_helper.h"

uint8_t pwm_channel;

static void set_pinMode() {
  pinMode(PIN_BUTTON_IN, INPUT_PULLDOWN);
  pinMode(PIN_BUTTON_OUT, OUTPUT);

  // pinMode(PIN_SWITCH1_LED, OUTPUT);
  // digitalWrite(PIN_SWITCH1_LED, LOW);
  init_pwm();
  pwm_channel = ISR_PWM.setPWM_Period(PIN_SWITCH1_LED, 600, 100.0); //600ms -> 100bpm
  ISR_PWM.disable(pwm_channel); // power off at start
  analogWrite(PIN_SWITCH1_LED, -1);

  pinMode(PIN_SWITCH1_IN, INPUT_PULLDOWN);
  // pinMode(PIN_SWITCH1_OUT, OUTPUT); // Switch 1 is wired to 3.3V directly

  pinMode(PIN_SWITCH2_IN, INPUT_PULLDOWN);
  pinMode(PIN_SWITCH2_OUT, OUTPUT);

  pinMode(PIN_SWITCH3_IN, INPUT_PULLDOWN);
  pinMode(PIN_SWITCH3_OUT, OUTPUT);
}

static void set_led_on() {
  ISR_PWM.enable(pwm_channel);
  ISR_PWM.modifyPWMChannel_Period(pwm_channel, PIN_SWITCH1_LED, 1000, 100.0);
}

static void set_led_blinking(const uint32_t& period_ms, const float& dutycycle) {
  ISR_PWM.enable(pwm_channel);
  ISR_PWM.modifyPWMChannel_Period(pwm_channel, PIN_SWITCH1_LED, period_ms, dutycycle);
}

static void set_led_off() {
  ISR_PWM.disable(pwm_channel);
  analogWrite(PIN_SWITCH1_LED,-1);
}

static void set_output_poweron() {
  digitalWrite(PIN_BUTTON_OUT, HIGH);
  // digitalWrite(PIN_SWITCH1_OUT, HIGH); // Switch 1 is wired to 3.3V directly
  digitalWrite(PIN_SWITCH2_OUT, HIGH);
  digitalWrite(PIN_SWITCH3_OUT, HIGH);
}

// set_pinmode();
// set_output_poweron();