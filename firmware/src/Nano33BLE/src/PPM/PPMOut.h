#ifndef CH_PPM_OUT2
#define CH_PPM_OUT2

#include "config.h"

void PpmOut_setPin(int pinNum);
void PpmOut_setChannel(int chan, uint16_t val);
void PpmOut_setChnCount(int chans);
void PpmOut_setInverted(bool inv);
void PpmOut_execute();
int PpmOut_getChnCount();

extern volatile bool interrupt;

 #endif