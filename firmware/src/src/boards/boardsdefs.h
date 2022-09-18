#pragma once

#define NRFPIN(port, pin) ((32 * port) + pin)
#define PIN_TO_NRFPORT(pin) (pin / 32)
#define PIN_TO_NRFPIN(pin) (pin % 32)
#define END_IO_PINS PIN(COUNT, -1, "\0")
#define PIN_NAME_TO_NUM(pin) PinNumber[pin]

extern const char* StrPins[];
