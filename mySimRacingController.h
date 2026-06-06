#include "pin_setup_helper.h"
#include <SimRacingController.h>

static const int gpioPins[4] = { PIN_BUTTON_IN, PIN_SWITCH1_IN, PIN_SWITCH2_IN, PIN_SWITCH3_IN };

void myControllerPinSetup(SimRacingController myController) {
  // myController.setMatrix(rowPins, MATRIX_ROWS, colPins, MATRIX_COLS);
  myController.setGpio(gpioPins, 4);
  // myController.setEncoders(encoderPinsA, encoderPinsB, encoderBtnPins, NUM_ENCODERS);
  myController.setProfiles(1);
  myController.setDebounceTime(50, 5);  // matrix/gpio=50ms, encoder=5ms
}

void myControllerPowerOn(SimRacingController myController) {
  set_pinMode();
  set_output_poweron();
  myController.begin();
  pinMode(PIN_SWITCH1_IN, INPUT_PULLDOWN);
}

// 1. create an instance of SimRacingController
// 2. myControllerPinSetup()
// 3. define and assign callbacks
// 4. myControllerPowerOn()

// example callback functions
  // void onMatrixChange(int profile, int row, int col, bool state) {}
  // void onGpioChange(int profile, int gpio, bool state) {}
  // void onEncoderChange(int profile, int encoder, int direction) {}
  // void onEncoderButtonChange(int profile, int encoder, bool state) {}

// example callback assignment
  // controller.setMatrixCallback(onMatrixChange);
  // controller.setGpioCallback(onGpioChange);
  // controller.setEncoderCallback(onEncoderChange);
  // controller.setEncoderButtonCallback(onEncoderButtonChange);

