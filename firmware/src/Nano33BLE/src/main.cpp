#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <platform/Callback.h>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include <chrono>
#include "PPMOut.h"
#include "dataparser.h"
#include "trackersettings.h"
#include "Wire.h"
#include "sense.h"

using namespace rtos;
using namespace mbed;

// Threads and IO
Thread btThread;
Thread dataThread;
Thread senseThread;
Ticker ioTick;

// GLOBALS
PpmOut *ppmout = nullptr;
TrackerSettings trkset;
volatile bool buttonpressed=false;

// Reset Button Pressed Flag on Read
bool wasButtonPressed() {
  if(buttonpressed) {
    __disable_irq();
    buttonpressed = false;
    __enable_irq();
  }
}

void bt_Thread() {
  while(true) {
    /* Add Bluetooth functionality Here */


    ThisThread::sleep_for(std::chrono::milliseconds(100));
  }
};

// Any IO Related Tasks, buttons, etc.. ISR. Run at 1Khz
void io_Task()
{
  static int i =0;
  // Fast Blink to know it's running
  if(i==100) {
    digitalWrite(LED_BUILTIN, HIGH);
  } 
  if(i==200) {
    digitalWrite(LED_BUILTIN, LOW);
    i=0;
  }
  i++;

  // Check button inputs, set flag, could make this an ISR but button for sure will be down for at least 1ms, also debounces
  if(digitalRead(trkset.buttonPin()) == 0) 
    buttonpressed = true; 
}

void setup() {
  // Setup LEDS
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR,HIGH);
  digitalWrite(LEDG,HIGH);
  digitalWrite(LEDB,HIGH);

  Serial.begin(1000000); // 1 Megabaud

  // Read the Settings from Flash, PPM Output Is created here.
  trkset.loadFromEEPROM(&ppmout);

  // Start the Data Thread
  dataThread.start(mbed::callback(data_Thread));
  dataThread.set_priority(osPriorityNormal);

  // Serial Read Ready Interrupt
  Serial.attach(&serialrx_Int);

  // Start the BT Thread, Higher Prority than data.
  btThread.start(mbed::callback(bt_Thread)); 
  btThread.set_priority(osPriorityNormal1);

  // Start the IO task at 1khz, Realtime priority
  ioTick.attach(mbed::callback(io_Task),std::chrono::milliseconds(1));
  
  // Start the sensor thread, Realtime priority
  sense_Init();
  senseThread.start(mbed::callback(sense_Thread));
  senseThread.set_priority(osPriorityRealtime); 
}

// Not Used
void loop() {}