#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
#define HAS_NOTIFYLED
#define HAS_CENTERBTN_ACTIVELOW

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_AN0 = 0
//     - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_GPORT & PIN_TO_GPIN
//     - The string descrition for D2 would be StrPins[IO_D2]

#define PIN_X \
  PIN(LED,          RP2040PIN(25), "Notification LED") \
  PIN(CENTER_BTN,   RP2040PIN(22), "Center Button") \
  END_IO_PINS \

typedef enum {
#define PIN(NAME, PINNO, DESC) IO_##NAME,
  PIN_X
#undef PIN
} pins_e;

const int8_t PinNumber[] = {
#define PIN(NAME, PINNO, DESC) PINNO,
    PIN_X
#undef PIN
};

// Required pin setting functions
#define pinMode(pin, mode) gpio_pin_configure(gpios[0], PIN_NAME_TO_NUM(pin), mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[0], PIN_NAME_TO_NUM(pin), value)
#define digitalRead(pin) gpio_pin_get(gpios[0], PIN_NAME_TO_NUM(pin))

// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 2.5f