#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
#define HAS_3DIODE_RGB
#define HAS_NOTIFYLED
#define HAS_LSM6DS3
#define HAS_CENTERBTN

// Mapping Analog numbers to Analog pins
#define AN0 0 // A0 pin
#define AN1 1 // A1 pin
#define AN2 4 // A2 pin
#define AN3 5 // A3 pin

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_D2 = 0
//     - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_NRFPORT & PIN_TO_NRFPIN
//     - The string descrition for D2 would be StrPins[IO_D2]

#define PIN_X \
  PIN(AN0,          NRFPIN(0,  2), "Analog 0 (AIN_0)") \
  PIN(AN1,          NRFPIN(0,  3), "Analog 1 (AIN_1)") \
  PIN(AN2,          NRFPIN(0, 28), "Analog 2 (AIN_4)") \
  PIN(AN3,          NRFPIN(0, 29), "Analog 3 (AIN_5)") \
  PIN(CENTER_BTN,   NRFPIN(1, 13), "Center Button, D8") \
  PIN(LEDR,         NRFPIN(0, 26), "Reg LED") \
  PIN(LEDG,         NRFPIN(0, 30), "Green LED") \
  PIN(LEDB,         NRFPIN(0,  6), "Blue LED") \
  PIN(LED,          NRFPIN(0, 17), "Notification LED") \
  PIN(PPMOUT,       NRFPIN(1, 15), "PPM Output Pin [D10]") \
  PIN(PPMIN,        NRFPIN(1, 14), "PPM In Pin [D9]") \
  PIN(TX,           NRFPIN(1, 11), "UART Transmit [TX]")  \
  PIN(RX,           NRFPIN(1, 12), "UART Receive [RX]") \
  PIN(TXINV,        NRFPIN(0, 27), "UART TX, Inv Pin") \
  PIN(RXINVO,       NRFPIN(0, 10), "UART RX, Out Inv") \
  PIN(RXINVI,       NRFPIN(0,  9), "UART RX, Inp Inv") \
  PIN(SCK,          NRFPIN(0, 21), "SPI Clock - Flash") \
  PIN(MOSI,         NRFPIN(0, 20), "SPI Data Out - Flash") \
  PIN(MISO,         NRFPIN(0, 24), "SPI Data In - Flash") \
  PIN(FLSH_WP,      NRFPIN(0, 22), "Flash Write Protect") \
  PIN(FLSH_HLD,     NRFPIN(0, 23), "Flash Hold") \
  PIN(I2CSDA,       NRFPIN(0,  4), "I2C - SDA") \
  PIN(I2CSCL,       NRFPIN(1,  5), "I2C - SCL") \
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
