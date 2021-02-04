#ifndef CH_PPM_IN
#define CH_PPM_IN

#include "config.h"

void PpmIn_setPin(int pinNum);
int PpmIn_getChannels(uint16_t *ch);
void PpmIn_setInverted(bool inv);
void PpmIn_execute();

 #endif