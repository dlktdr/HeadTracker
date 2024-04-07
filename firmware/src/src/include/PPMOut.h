#pragma once

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#endif

#include <stdint.h>

int PpmOut_init();
void PpmOut_setPin(int pinNum);
void PpmOut_setChannel(int chan, uint16_t val);
void PpmOut_execute();
int PpmOut_getChnCount();

extern volatile bool interrupt;
