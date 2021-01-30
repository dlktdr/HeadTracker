#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <platform/Callback.h>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include <chrono>

#include "PPM/PPMOut.h"
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

const char *FW_VERSION = "0.4";
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

FlashIAP flash;

void setup() { 
    // Setup Serial
    serial_Init();
    
    // Startup delay to get serial connected to see any startup issues
    delay(1000);

    // Setup Pins
    io_Init();

    // Read the Settings from Flash
    flash_Init();   

    // Start the BT Thread, Higher Prority than data.
    bt_Init();

    trkset.loadFromEEPROM();
       
    // Serial Read Ready Interrupt
    Serial.attach(&serialrx_Int);    
    
    sense_Init();
    
    // Start the IO task at 1khz interrupt
    ioTick.attach(callback(io_Task),std::chrono::milliseconds(IO_PERIOD));

    // Setup Event Queue
    queue.call_in(std::chrono::milliseconds(10),sense_Thread);
    queue.call_in(std::chrono::milliseconds(SERIAL_PERIOD),serial_Thread);
    queue.call_in(std::chrono::milliseconds(BT_PERIOD),bt_Thread);
    queue.call_in(std::chrono::milliseconds(DATA_PERIOD),data_Thread);
    queue.dispatch_forever();    
}

// Not Used
void loop() {}  
