#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
//#define HAS_3DIODE_RGB
//#define HAS_NOTIFYLED
#define HAS_LSM6DS3
#define HAS_CENTERBTN
#define HAS_PPMIN
#define HAS_PPMOUT

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_D2 = 0
//     - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_NRFPORT & PIN_TO_NRFPIN
//     - The string descrition for D2 would be StrPins[IO_D2]

#define PIN_X \
  PIN(CENTER_BTN,   ESPPIN(1), "Center Button") \
  PIN(LED,          ESPPIN(2), "Notification LED") \
  PIN(PPMOUT,       ESPPIN(3), "PPM Output Pin [D10]") \
  PIN(PPMIN,        ESPPIN(4), "PPM In Pin [D9]") \
  PIN(TX,           ESPPIN(5), "UART Transmit [TX]")  \
  PIN(RX,           ESPPIN(6), "UART Receive [RX]") \
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

// TODO Find good values here
// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 2.5f
