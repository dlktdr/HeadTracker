#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board features
#define HAS_WS2812
#define HAS_BUZZER
#define HAS_NOTIFYLED
#define HAS_MPU6500
#define HAS_QMC5883
#define HAS_CENTERBTN
#define AN0 1 // Battery V on analog 0
#define AN1 5 // Pin 0.29
#define AN2 0 // Pin 0.02
#define AN3 4 // Pin 0.28

// Pins (name, number, description)
// NOTE: These pins are an enum entry. IO_ is prepended e.g. IO_AN0 = 0
//     - Use PIN_NAME_TO_NUM(IO_AN0) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_NRFPORT & PIN_TO_NRFPIN
//     - The string descrition for AN0 would be StrPins[IO_AN0]

#define PIN_X \
  PIN(AN0,          NRFPIN(0,  3), "Analog Battery Voltage") \
  PIN(AN1,          NRFPIN(0, 29), "Analog 1 (AIN_5)") \
  PIN(AN2,          NRFPIN(0,  2), "Analog 2 (AIN_0)") \
  PIN(AN3,          NRFPIN(0, 28), "Analog 3 (AIN_4)") \
  PIN(CENTER_BTN,   NRFPIN(1, 13), "Center Button") \
  PIN(LEDWS,        NRFPIN(1, 10), "WS2812 LED") \
  PIN(LED,          NRFPIN(0, 13), "Notification LED") \
  PIN(PPMOUT,       NRFPIN(0, 31), "PPM Output Pin") \
  PIN(PPMIN,        NRFPIN(0, 30), "PPM In Pin") \
  PIN(BUZZ,         NRFPIN(1, 11), "Buzzer") \
  PIN(TX,           NRFPIN(0,  9), "UART Transmit")  \
  PIN(RX,           NRFPIN(0, 10), "UART Receive") \
  PIN(TXINV,        NRFPIN(0,  4), "UART TX, Inv Pin") \
  PIN(RXINVO,       NRFPIN(0,  0), "UART RX, Out Inv") \
  PIN(RXINVI,       NRFPIN(0,  1), "UART RX, Inp Inv") \
  PIN(I2CSDA,       NRFPIN(0,  5), "I2C - SDA") \
  PIN(I2CSCL,       NRFPIN(1,  9), "I2C - SCL") \
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
#define pinMode(pin, mode) gpio_pin_configure(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)), mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)), value)
#define digitalRead(pin) gpio_pin_get(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)))

// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 3.5
