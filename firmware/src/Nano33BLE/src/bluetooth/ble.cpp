#include <Arduino.h>
#include <ArduinoBLE.h>
#include "ble.h"
#include "../PPM/PPMOut.h"
#include "../serial.h"
#include "../main.h"
#include "../dataparser.h"

using namespace mbed;
using namespace events;

//-----------------------------------------------------

void bt_Thread()
{
    digitalWrite(LEDB,HIGH);

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
        if(bt->isConnected())
            digitalWrite(LEDB,LOW);
        else
            digitalWrite(LEDB,HIGH);
    }

    // Reduces BT Data Rate while UI connected.. Issues with BLE lib
    float btperiod = BT_PERIOD;
    if(trkset.blueToothMode() == BTPARAHEAD) { // Head board
        if(uiconnected) //Slow down transmission rate while connected to the GLUquadric
            btperiod *= 5.0;
    } else if(trkset.blueToothMode() == BTPARARMT) {  // Remote board.. Need to call poll often
        // Full Speed, 40hz need to go fast as possible
        btperiod *= 1.0;
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
    for(int i=0;i < BT_CHANNELS;i++) {
        chan_vals[i] = TrackerSettings::PPM_CENTER;
    }
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
    if(count >= 0 && count <= BT_CHANNELS)
        num_chans = count;
}





