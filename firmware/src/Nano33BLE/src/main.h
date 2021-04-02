#ifndef MAIN_H
#define MAIN_H

#include "mbed.h"
#include "trackersettings.h"
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"

using namespace mbed;
using namespace events;

#define DATA_PERIOD 120
#define BT_PERIOD 40.0f      // Bluetooth update rate
#define SERIAL_PERIOD 20  // Serial processing
#define IO_PERIOD 20      // milliseconds
#define SENSE_PERIOD 9000 //100hz Update Rate
#define SBUS_PERIOD 40

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

