#pragma once

#include <stdint.h>

extern volatile bool pauseForFlash;

int socReadFlash(char *dataout, int len);
int socWriteFlash(const uint8_t *data, int len);
void socClearFlash();
const uint8_t *socGetMMFlashPtr();

