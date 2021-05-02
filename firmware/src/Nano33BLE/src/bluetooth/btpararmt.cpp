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
#include <ArduinoJson.h>
#include "ArduinoBLE.h"
#include "serial.h"
#include "io.h"
#include "main.h"
#include "trackersettings.h"
#include "opentxbt.h"
#include "btpararmt.h"

Timer watchdog;

#define WATCHDOG_TIMEOUT 10000

uint16_t chanoverrides=0xFFFF; // Default to all enabled
bool bleconnected=false;

void fff6Written(BLEDevice central, BLECharacteristic characteristic);
void overrideWritten(BLEDevice central, BLECharacteristic characteristic);

// Should be safe, never should be more than one instance of this
BTParaRmt *BTParaInst = nullptr;

BTParaRmt::BTParaRmt() : BTFunction()
{
    BTParaInst = this;
    bleconnected = false;
    chanoverrides = 0xFFFF;

    // Reset all BT channels to disabled
    for(int i = 0; i <16; i++)
        chan_vals[i] = 0;

    serialWriteln("HT: Starting Remote Para Bluetooth");

    // Save Local Address
    strcpy(_address,BLE.address().c_str());

    watchdog.start();
}

BTParaRmt::~BTParaRmt()
{
    BTParaInst = nullptr;
    // Reset all BT channels to center
    for(int i = 0; i <16; i++)
        chan_vals[i] = TrackerSettings::PPM_CENTER;

    serialWriteln("HT: Stopping Remote Para Bluetooth");

    // Disconnect
    BLE.disconnect();}

// Set channel does nothing on a BT receiver (Remote board)
void BTParaRmt::setChannel(int channel, const uint16_t value)
{

}

// If not connected, not valid, or channel isn't overriden return zero (disabled)
uint16_t BTParaRmt::getChannel(int channel)
{
    if(channel >= 0 &&
       channel < BT_CHANNELS &&
       bleconnected == true &&
       (1 << channel) & chanoverrides)
            return chan_vals[channel];
    return 0;
}

void BTParaRmt::execute()
{
     // Start Scan for PARA Slaves
    if(!BLE.connected() && !scanning) {
        serialWriteln("BRMT: Starting Scan");
        BLE.scan();
        scanning = true;
        bleconnected = false;
    }

    // If scanning see if there is a BLE device available
    if(!BLE.connected() && scanning) {
        bool fault = false;
        peripheral = BLE.available();
        if(peripheral) {
#ifdef DEBUG
            if (peripheral.address() == "") {
                serialWrite("BRMT:  <no advertised address> ");
            } else {
                serialWrite("BRMT: Advertised Device Address: ");
                serialWrite(peripheral.address());
            }
            if (peripheral.localName() == "") {
                serialWrite(" <no advertised local name> ");
            } else {
                serialWrite(" Local Name: ");
                serialWrite(peripheral.localName());
            }

            if (peripheral.advertisedServiceUuid() == "") {
                serialWrite(" <no advertised service UUID> ");
            } else {
                serialWrite(" Advertised Service UUID ");
                serialWrite(peripheral.advertisedServiceUuid());
            }
            serialWriteln("");
#endif
            if(peripheral.localName() == "Hello" &&
               peripheral.advertisedServiceUuid() == "fff0") {

#ifdef DEBUG
                serialWriteln("BRMT: Found a PARA device");
                serialWriteln("BRMT: Stopping scan");
#endif
                BLE.stopScan();
                scanning = false;
#ifdef DEBUG
                serialWriteln("BRMT: Connecting...");
#endif
                if(peripheral.connect()) {
                    serialWriteln("BRMT: Connected");
                    ThisThread::sleep_for(std::chrono::milliseconds(150));
#ifdef DEBUG
                    serialWriteln("BRMT: Discovering Attributes");
#endif
                    if(peripheral.discoverAttributes()) {
#ifdef DEBUG
                        serialWriteln("BRMT: Discovered Attributes");
#endif
                        fff6 = peripheral.service("fff0").characteristic("fff6");
                        if(fff6) {
#ifdef DEBUG
                            serialWriteln("BRMT: Attaching Event Handler");
#endif
                            fff6.setEventHandler(BLEWritten, fff6Written);  // Call this function on data received
                            serialWriteln("BRMT: Found channel data characteristic");
#ifdef DEBUG
                            serialWriteln("BRMT: Subscribing...");
#endif
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(fff6.subscribe()) {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribed to data!");
#endif
                            } else {
                                serialWriteln("BRMT: Subscribe to data failed");
                                fault = true;
                            }
                        } else  {
#ifdef DEBUG
                            serialWriteln("BRMT: Couldn't find characteristic");
#endif
                            fault = true;
                        }
                        // If this is a Headtracker it may have a reset center option
                        butpress = peripheral.service("fff1").characteristic("fff2");
                        if(butpress) {
#ifdef DEBUG
                            serialWriteln("BRMT: Tracker has ability to remote reset center");
#endif
                        }
                        overridech = peripheral.service("fff1").characteristic("fff1");
                        if(overridech) {
                            overridech.setEventHandler(BLEWritten, overrideWritten);  // Call this function on data received
                            // Initial read of overridden channels
                            overridech.readValue(chanoverrides);
#ifdef DEBUG
                            serialWriteln("BRMT: Tracker has sent the channels it wants overriden");
#endif
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(overridech.subscribe()) {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribed to channel overrides!");
#endif
                            } else {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribe to override Failed");
#endif
                                fault = true;
                            }
                        } else
                            chanoverrides = 0xFFFF;
                    } else {
                        serialWriteln("BRMT: Attribute Discovery Failed");
                        fault = true;
                        }
                } else {
                    serialWriteln("BRMT: Couldn't connect to Para Slave, Rescanning");
                    fault = true;
                }
            }
        }
        // On any faults, disconnect and start scanning again
        if(fault) {
            peripheral.disconnect();
            BLE.scan();
            scanning = true;
        }
    }

    // Connected
    if(BLE.connected()) {
        // Check how long we have been connected but haven't received any data
        // if longer than timeout, disconnect.
        uint32_t wdtime = std::chrono::duration_cast<std::chrono::milliseconds>(watchdog.elapsed_time()).count();
        if(wdtime > WATCHDOG_TIMEOUT) {
            serialWriteln("BRMT: ***WATCHDOG.. Forcing disconnect. No data received");
            BLE.disconnect();
            bleconnected = false;
        }
    // Not Connected
    } else {
        bleconnected = false;
        watchdog.reset();
    }

    BLE.poll();
}

void overrideWritten(BLEDevice central, BLECharacteristic characteristic)
{
    characteristic.readValue(chanoverrides);
}

// Called when Radio Outputs new Data
void fff6Written(BLEDevice central, BLECharacteristic characteristic) {
    // Got Data Must Be Connected
    bleconnected = true;

    // Got some data clear the watchdog timer
    watchdog.reset();

    uint8_t buffer1[BLUETOOTH_LINE_LENGTH+1];
    int len = characteristic.readValue(buffer1,32);

    // Simulate sending byte by byte like opentx uses, stores in global
    for(int i=0;i<len;i++) {
        processTrainerByte(buffer1[i]);
    }

    // Store all channels
    for(int i=0 ; i < BT_CHANNELS; i++) {
        // Only set the data on channels that are allowed to be overriden
        if(BTParaInst) {
            BTParaInst->chan_vals[i]  = BtChannelsIn[i];
        }
    }

#ifdef DEBUG
    Serial.print("OR: ");
    printHex(chanoverrides);
    Serial.print("|");
    for(int i=0;i<CHANNEL_COUNT;i++) {
        Serial.print("Ch");Serial.print(i+1);Serial.print(":");Serial.print(ppmChannels[i]);Serial.print(" ");
    }
    Serial.println("");
#endif
}

void BTParaRmt::sendButtonData(char bd)
{
    if(butpress)
        butpress.writeValue((uint8_t)bd);
}