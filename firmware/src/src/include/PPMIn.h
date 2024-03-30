#pragma once

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#endif

#include <stdint.h>

int PpmIn_init();
void PpmIn_setPin(int pinNum);
int PpmIn_getChannels(uint16_t *ch);
void PpmIn_execute();
