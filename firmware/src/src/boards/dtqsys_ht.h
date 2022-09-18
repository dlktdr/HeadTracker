#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_AN0 = 0
//     - Use PIN_NAME_TO_NUM(IO_AN0) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_NRFPORT & PIN_TO_NRFPIN
//     - The string descrition for AN0 would be StrPins[IO_AN)]

#define PIN_X \
  PIN(AN0,          NRFPIN(0,  3), "Analog Battery Voltage") \
  PIN(CENTER_BTN,   NRFPIN(0, 29), "Center Button") \
  PIN(LEDWS,        NRFPIN(1, 10), "WS2812 LED") \
  PIN(PPMOUT,       NRFPIN(0, 31), "PPM Output Pin") \
  PIN(PPMIN,        NRFPIN(0, 13), "PPM In Pin") \
  PIN(BUZZ,         NRFPIN(1, 11), "Buzzer") \
  PIN(TX,           NRFPIN(0,  9), "UART Transmit")  \
  PIN(RX,           NRFPIN(0, 10), "UART Receive") \
  PIN(TXINV,        NRFPIN(0, 15), "UART TX, Inv Pin") \
  PIN(RXINVO,       NRFPIN(1, 13), "UART RX, Out Inv") \
  PIN(RXINVI,       NRFPIN(0,  2), "UART RX, Inp Inv") \
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

