#ifndef HEXPRINT_H
#define HEXPRINT_H

#include <stdint.h>

char *bytesToHex(const uint8_t *data, int len, char *buffer);

#endif