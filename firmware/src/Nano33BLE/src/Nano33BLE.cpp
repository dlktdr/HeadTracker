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

// Version 1.0 - NANO33BLE
const char *FW_VERSION = "1.0";
const char *FW_BOARD = "NANO33BLE";

using namespace rtos;
using namespace mbed;

// Threads and IO
Thread btThread(osPriorityNormal1,OS_STACK_SIZE*3);
Thread dataThread(osPriorityNormal,OS_STACK_SIZE*3);
Thread serialThread(osPriorityNormal2);
Thread senseThread(osPriorityRealtime);
Ticker ioTick;

// GLOBALS
TrackerSettings trkset;
Mutex dataMutex;
Mutex eepromWait;
ConditionVariable eepromWriting(eepromWait);
volatile bool pauseForEEPROM=false;

FlashIAP flash;

void setup() { 
  // Setup Serial Port
  Serial.begin(921600);
  //Serial.begin(115200);
  
  init_Flash();   
  io_Init();

  // Read the Settings from Flash
  trkset.loadFromEEPROM();
  
  // Start the Data Thread
  dataThread.start(callback(data_Thread));
  
  // Serial Read Ready Interrupt
  Serial.attach(&serialrx_Int);    
  serialThread.start(callback(serial_Thread));

  // Start the BT Thread, Higher Prority than data.
  bt_Init();
  btThread.start(callback(bt_Thread)); 
  
  // Start the IO task at 1khz, Realtime priority
  ioTick.attach(callback(io_Task),std::chrono::milliseconds(1));

  // Start the sensor thread, Realtime priority
  if(!sense_Init()) {
    senseThread.start(callback(sense_Thread));    
  }
}

// Not Used
void loop() {}