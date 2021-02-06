#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <platform/Callback.h>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include <chrono>

#include "PPM/PPMOut2.h"
#include "PPM/PPMIn.h"
#include "dataparser.h"
#include "trackersettings.h"
#include "Wire.h"
#include "sense.h"
#include "ble.h"
#include "io.h"
#include "flash.h"
#include "serial.h"
#include "main.h"


const char *FW_VERSION = "0.42";
const char *FW_BOARD = "NANO33BLE";

using namespace rtos;
using namespace mbed;
using namespace events;

// Event Queue
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Ticker ioTick;

// GLOBALS
TrackerSettings trkset;
Mutex dataMutex;
Mutex eepromWait;
ConditionVariable eepromWriting(eepromWait);
volatile bool pauseThreads=false;

volatile bool dataready=false;
uint32_t buffer[20];
int bufindex=0;

void setup() 
{
    // Setup Serial
    serial_Init();

    // Startup delay to get serial connected & see any startup issues
    delay(4000);

    PpmOut_setChnCount(8);
    PpmOut_setChannel(0,1800);
    PpmOut_setPin(D8);


    // Setup Pins - io.cpp
    io_Init();

   /* // Read the Settings from Flash - flash.cpp
    flash_Init();

    // Start the BT Thread, Higher Prority than data. - bt.cpp
    bt_Init();
         
    // Actual Calculations - sense.cpp
    sense_Init();

    // Load settings from flash - trackersettings.cpp
    trkset.loadFromEEPROM();
    
    // --- Starts all Events & ISR's Below ---

    // Serial Read Ready Interrupt - serial.cpp
    Serial.attach(&serialrx_Int);
*/
    // Start the IO task at 100hz interrupt
    ioTick.attach(callback(io_Task),std::chrono::milliseconds(IO_PERIOD));
/*
    // Setup Event Queue
    queue.call_in(std::chrono::milliseconds(10),sense_Thread);
    queue.call_in(std::chrono::milliseconds(SERIAL_PERIOD),serial_Thread);
    queue.call_in(std::chrono::milliseconds(BT_PERIOD),bt_Thread);
    queue.call_in(std::chrono::milliseconds(DATA_PERIOD),data_Thread);

    // Start everything
    queue.dispatch_forever();*/
}

// Not Used
void loop() 
{
    delay(100);
    Serial.print("Interrupt "); 
    Serial.println(interrupt=true?"YES":"NO");
}
