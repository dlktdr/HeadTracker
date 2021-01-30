#ifndef MAIN_H
#define MAIN_H

#include "mbed.h"
#include "trackersettings.h"
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"

using namespace mbed;
using namespace events;

#define DATA_PERIOD 150
#define BT_PERIOD 40
#define SERIAL_PERIOD  15
#define IO_PERIOD 1 // milliseconds
#define SENSE_PERIOD 7000 //microseconds

// Globals
extern TrackerSettings trkset;
extern Mutex dataMutex;
extern Mutex eeMutex;
extern EventQueue queue;
extern FlashIAP flash;
bool wasButtonPressed();
void pressButton();
extern const char *FW_VERSION;
extern const char *FW_BOARD;

// Pauses Threads if Set True to allow
extern volatile bool pauseThreads;

#endif

