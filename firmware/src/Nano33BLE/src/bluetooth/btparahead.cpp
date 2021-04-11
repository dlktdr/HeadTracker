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
#include "ArduinoBLE.h"
#include "trackersettings.h"
#include "btparahead.h"
#include "serial.h"
#include "io.h"
#include "main.h"

//-------------------------------------------
// FRSKY Para Wireless Traniner interface
// See https://github.com/ysoldak/HeadTracker

BTParaHead::BTParaHead() : BTFunction()
{
    bleconnected = false;

    para = new BLEService("FFF0");

    fff6 = new BLECharacteristic("FFF6", BLEWriteWithoutResponse | BLENotify | BLEAutoSubscribe, 32);

    rmbrd = new BLEService("FFF1");
    rbfff1 = new BLECharacteristic("FFF1", BLERead | BLENotify, 2); // Overridden Channels 16bits
    rbfff2 = new BLEBoolCharacteristic("FFF2", BLEWrite );

    serialWriteln("HT: Starting Head Para Bluetooth");

    BLE.setConnectable(true);
    BLE.setLocalName("Hello");

    BLE.setAdvertisedService(*para);

    para->addCharacteristic(*fff6);
    BLE.addService(*para);

    // Remote Slave board Returned Information
    rmbrd->addCharacteristic(*rbfff1);
    rmbrd->addCharacteristic(*rbfff2);
    BLE.addService(*rmbrd);
    rbfff1->writeValue((uint16_t)0); // Channels overridden

    BLE.advertise();

    // Save Local Address
    strcpy(_address,BLE.address().c_str());
    crc = 0;
    bufferIndex = 0;
}

BTParaHead::~BTParaHead()
{
    serialWriteln("HT: Stopping Head Para Bluetooth");

    // Disconnect
    BLE.disconnect();

    // Delete Service 1
    delete fff6;
    delete para;

    // Delete Service 2
    delete rbfff2;
    delete rbfff1;
    delete rmbrd;
}

void BTParaHead::execute()
{
    // Disconnection
    if(bleconnected == true && !BLE.connected()) {
        serialWrite("HT: Disconnected event, central: ");
        serialWriteln(BLE.address().c_str());
        strcpy(_address,BLE.address().c_str());
        bleconnected = false;
    }

    // Connection
    if(bleconnected == false && BLE.connected()) {
        serialWrite("HT: Connected event, Address: ");
        serialWriteln(BLE.central().address().c_str());
        bleconnected = true;
    }

    // Connected
     if(bleconnected) {
        sendTrainer();

        // Get the returned data from the slave bluetooth board
        static bool rbprs = false;
        char butval;
        int len = rbfff2->readValue(&butval,1);
        if(len == 1) {
            if(butval == 'R' && rbprs == false) {
                serialWriteln("HT: Remote BT Button Pressed");
                pressButton();
                rbprs = true;
            } else if(butval != 'R' && rbprs == true) {
                rbprs = false;
            }
        }

        // Set the bits of the overridden channels, for remote PPM in/out
        static uint16_t last_ovridech = 0;
        uint16_t ovridech = 0;
        ovridech |= 1 << (trkset.tiltCh()-1);
        ovridech |= 1 << (trkset.rollCh()-1);
        ovridech |= 1 << (trkset.panCh()-1);
        if(last_ovridech != ovridech)
            rbfff1->writeValue(ovridech);
        last_ovridech = ovridech;
    }
}

void BTParaHead::sendTrainer()
{
    uint8_t output[BLUETOOTH_LINE_LENGTH+1];
    int len;
    len = setTrainer(output);
    if(len > 0 && fff6 != nullptr)
        fff6->writeValue(output,len);
}

// Head BT does not return BT data
uint16_t BTParaHead::getChannel(int channel)
{
    return 0;
}

// Part of setTrainer to calculate CRC
// From OpenTX

void BTParaHead::pushByte(uint8_t byte)
{
    crc ^= byte;
    if (byte == START_STOP || byte == BYTE_STUFF) {
        buffer[bufferIndex++] = BYTE_STUFF;
        byte ^= STUFF_MASK;
    }
    buffer[bufferIndex++] = byte;
}

/* Builds Trainer Data
*     Returns the length of the encoded PPM + CRC
*     Data saved into addr pointer
*/
int BTParaHead::setTrainer(uint8_t *addr)
{
    // Allocate Channel Mappings, Set Default to all Center
    uint8_t * cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
    for (int channel=0; channel < BT_CHANNELS; channel+=2, cur+=3) {
        uint16_t channelValue1 = chan_vals[channel];
        uint16_t channelValue2 = chan_vals[channel+1];

        pushByte(channelValue1 & 0x00ff);
        pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
        pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
    }

    buffer[bufferIndex++] = crc;
    buffer[bufferIndex++] = START_STOP; // end byte

    // Copy data to array
    memcpy(addr,buffer,bufferIndex);

    return bufferIndex;
}