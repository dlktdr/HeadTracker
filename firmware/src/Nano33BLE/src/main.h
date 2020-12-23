#ifndef MAIN_H
#define MAIN_H

#include "mbed.h"
#include "trackersettings.h"
#include "PPMOut.h"

// Globals
extern TrackerSettings trkset;
extern PpmOut *ppmout;
extern Mutex dataMutex;
bool wasButtonPressed();

#endif

