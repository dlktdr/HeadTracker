#pragma once

#include <nrfx_ppi.h>
#include <nrfx_gpiote.h>

void PpmIn_setPin(int pinNum);
int PpmIn_getChannels(uint16_t *ch);
void PpmIn_setInverted(bool inv);
void PpmIn_execute();
