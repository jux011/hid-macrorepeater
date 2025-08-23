#include "RP2040_Slow_PWM.h"

#define HW_TIMER_FREQ  10.0f  // base frequency of ISR_PWM
volatile uint64_t startMicros = 0;

RP2040_Timer ITimer(3);  // timer 0 occupied running usbh, use another

RP2040_Slow_PWM ISR_PWM;

bool TimerHandler(struct repeating_timer *t) {
  (void)t;

  ISR_PWM.run();

  return true;
}

void init_pwm() {
  if (ITimer.attachInterrupt(HW_TIMER_FREQ, TimerHandler)) {
    Serial.print(F("Starting ITimer OK"));
  } else {
    Serial.println(F("Can't set ITimer. Select another freq. or timer"));
  }
}