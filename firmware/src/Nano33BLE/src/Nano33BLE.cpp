/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "sense.h"
#include "ble.h"
#include "io.h"
#include "flash.h"
#include "serial.h"
#include "main.h"
#include "SBUS/uarte_sbus.h"
#include "PWM/pmw.h"

const char *FW_VERSION = "1.0";
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

volatile bool pauseThreads=false;
volatile bool dataready=false;

uint32_t buffer[20];
int bufindex=0;

void setup()
{
    // Setup Serial
    serial_Init();

    // Startup delay to get serial connected & see any startup issues
    delay(1000);

    // Setup Pins - io.cpp
    io_Init();

    // Read the Settings from Flash - flash.cpp
    flash_Init();

    // Start the BT Thread, Higher Prority than data. - bt.cpp
    bt_Init();

    // Actual Calculations - sense.cpp
    sense_Init();

    // ********************* !!!
    // set, #define SERIAL_HOWMANY		0 in pins_arduino.h to disable _UART1 on TX/RX pins first
    // Start SBUS - SBUS/uarte_sbus.cpp (Pins D0/TX, D1/RX)
    SBUS_Init(0,1);

    // PWM Outputs - Fixed to A0-A3
    PWM_Init(50); // Start PWM at 100 Hz update rate

    // Load settings from flash - trackersettings.cpp
    trkset.loadFromEEPROM();

    // --- Starts all Events & ISR's Below ---

    // Serial Read Ready Interrupt - serial.cpp
    Serial.attach(&serialrx_Int);

    // Start the IO task at 100hz interrupt
    ioTick.attach(callback(io_Task),std::chrono::milliseconds(IO_PERIOD));

    // Setup Event Queue
    queue.call_in(std::chrono::milliseconds(10),sense_Thread);
    queue.call_in(std::chrono::milliseconds(SERIAL_PERIOD),serial_Thread);
    queue.call_in(std::chrono::milliseconds((int)BT_PERIOD),bt_Thread);
    queue.call_in(std::chrono::milliseconds(DATA_PERIOD),data_Thread);
    queue.call_in(std::chrono::milliseconds(SBUS_PERIOD),SBUS_Thread);

    // Start everything
    queue.dispatch_forever();
}

// Not Used
void loop()
{
    ThisThread::sleep_for(std::chrono::milliseconds(100000));
}
