#pragma once

#include <stdint.h>

#define ESPPIN(pin) pin
#define RP2040PIN(pin) pin
#define NRFPIN(port, pin) ((32 * port) + pin)
#define PIN_TO_GPORT(pin) (pin / 32)
#define PIN_TO_GPIN(pin) (pin % 32)
#define PIN_NAME_TO_NUM(pin) PinNumber[pin]

extern const char* StrPins[];
extern const char* StrPinDescriptions[];