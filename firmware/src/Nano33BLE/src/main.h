#ifndef MAIN_H
#define MAIN_H

#include "mbed.h"
#include "trackersettings.h"
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"

using namespace mbed;

// Globals
extern TrackerSettings trkset;
extern PpmOut *ppmout;
extern PpmIn *ppmin;
extern Mutex dataMutex;
extern Mutex eeMutex;
extern FlashIAP flash;
bool wasButtonPressed();
void pressButton();
extern const char *FW_VERSION;
extern const char *FW_BOARD;

// Pauses Threads if Set True to allow
extern volatile bool pauseForEEPROM;

#endif

