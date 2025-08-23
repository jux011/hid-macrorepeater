#include "RP2040_Slow_PWM.h"

#define HW_TIMER_INTERVAL_US  10000L  // minimum period of ISR_PWM, in microseconds
volatile uint64_t startMicros = 0;

RP2040_Timer ITimer(3);  // timer 0 occupied running usbh, use another

RP2040_Slow_PWM ISR_PWM;

bool TimerHandler(struct repeating_timer *t) {
  (void)t;

  ISR_PWM.run();

  return true;
}

void init_pwm() {
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_US, TimerHandler)) {
    startMicros = micros();
    Serial.print(F("Starting ITimer OK, micros() = "));
    Serial.println(startMicros);
  } else {
    Serial.println(F("Can't set ITimer. Select another freq. or timer"));
  }
}