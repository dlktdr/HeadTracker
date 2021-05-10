#include <Arduino.h>
#include <ArduinoBLE.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "ble.h"
#include "PPM/PPMOut.h"
#include "serial.h"
#include "main.h"

using namespace mbed;
using namespace events;

#define BT_CHANNELS 8

//-----------------------------------------------------

void bt_Thread()
{
    // Blue LED Indicates bluetooth usage
    digitalWrite(LEDB,LOW);

    // Don't output bluetooth if threads paused
    if(pauseThreads) {
        queue.call_in(std::chrono::milliseconds(100),bt_Thread);
        return;
    }

    // Is BT Enabled
    BTFunction *bt = trkset.getBTFunc();
    if(bt != nullptr) {
        // Then call the execute function
        bt->execute();
    }

    digitalWrite(LEDB,HIGH);

    // Reduces BT Data Rate while UI connected.. Issues with BLE lib
    float btperiod = BT_PERIOD;
    if(trkset.blueToothMode() == BTPARA) { // Head board
        if(uiconnected) //Slow down transmission rate while connected to the GUI
            btperiod *= 10.0;
    }
    queue.call_in(std::chrono::milliseconds((int)btperiod),bt_Thread);
}

void bt_Init()
{
    if(!BLE.begin()) {
        serialWriteln("HT: Fault Starting Bluetooth");
        return;
    }
}

//
BTFunction::BTFunction()
{
    // Reset all 24 channels
    for(int i=0;i < 24;i++) {
        chan_vals[i] = 1500;
    }
    num_chans = TrackerSettings::DEF_PPM_CHANNELS;
    crc = 0;
    bufferIndex = 0;
}

BTFunction::~BTFunction()
{

}

// Common to all classes, sets the channel values
void BTFunction::setChannel(int channel, uint16_t value)
{
    // Allowed 0-7 Channels
    if(channel >= 0 && channel < BT_CHANNELS)
        chan_vals[channel] = value;
}

// Sets channel count
void BTFunction::setChannelCount(int count)
{
    if(count >= 0 && count < BT_CHANNELS)
        num_chans = count;
}

// Part of setTrainer to calculate CRC
// From OpenTX

void BTFunction::pushByte(uint8_t byte)
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
int BTFunction::setTrainer(uint8_t *addr)
{
    // Allocate Channel Mappings, Set Default to all Center
    uint8_t * cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
    for (int channel=0; channel < num_chans; channel+=2, cur+=3) {
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


