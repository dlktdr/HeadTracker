#pragma once

#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>


void PpmOut_setPin(int pinNum);
void PpmOut_setChannel(int chan, uint16_t val);
void PpmOut_execute();
int PpmOut_getChnCount();

extern volatile bool interrupt;
