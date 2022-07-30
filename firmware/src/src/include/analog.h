#ifndef ANALOG_IN_H
#define ANALOG_IN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float analogRead(int channel);
#define BAD_ANALOG_READ -123

#ifdef __cplusplus
}
#endif

#endif