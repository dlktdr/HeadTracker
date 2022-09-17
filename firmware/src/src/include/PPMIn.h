#pragma once

#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>


void PpmIn_setPin(int pinNum);
int PpmIn_getChannels(uint16_t *ch);
void PpmIn_execute();
