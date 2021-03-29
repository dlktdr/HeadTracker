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
uint16_t BTParaHead::getChannel(int channel, bool &valid)
{
    valid = false;
    return TrackerSettings::DEF_CENTER;
}