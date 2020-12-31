#ifndef MAIN_H
#define MAIN_H

#include "mbed.h"
#include "trackersettings.h"
#include "PPMOut.h"

// Globals
extern TrackerSettings trkset;
extern PpmOut *ppmout;
extern Mutex dataMutex;
extern Mutex eeMutex;
extern FlashIAP flash;
bool wasButtonPressed();
void pressButton();
extern const char *FW_VERSION;
extern const char *FW_BOARD;

// Pauses Threads if Set True to allow
// Flash to write uninterrupted, too slow
// 
extern volatile bool pauseForEEPROM;

#endif

