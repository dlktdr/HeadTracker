#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
//#define HAS_LSM6DS3
#define HAS_CENTERBTN_ACTIVELOW
#define HAS_NOTIFYLED
#define HAS_MPU6886
#define HAS_BUZZER
#define HAS_POWERHOLD
//#define HAS_PPMIN
//#define HAS_PPMOUT

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_D2 = 0
//     - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
//     - The string descrition for D2 would be StrPins[IO_D2]

// TODO: ** Replace these with devicetree overlay files

#define PIN_X \
  PIN(CENTER_BTN,   ESPPIN(37), "Center Button") \
  PIN(LED,          ESPPIN(19), "Notification LED") \
  PIN(TX,           ESPPIN(32), "UART Transmit")  \
  PIN(RX,           ESPPIN(33), "UART Receive") \
  PIN(PWRHOLD,      ESPPIN( 4), "Hold High=Powered") \
  PIN(BUZZ,         ESPPIN( 2), "Buzzer") \
  END_IO_PINS \

/*
  PIN(PPMOUT,       ESPPIN(10),"PPM Output Pin") \
  PIN(PPMIN,        ESPPIN(9), "PPM In Pin") \
*/

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

#define pinMode(pin, mode) gpio_pin_configure(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)), mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)), value)
#define digitalRead(pin) gpio_pin_get(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)))

// TODO Find good values here
// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 2.5f
