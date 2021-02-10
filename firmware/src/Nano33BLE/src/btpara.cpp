#include "ArduinoBLE.h"
#include <Arduino.h>
#include "btpara.h"
#include "serial.h"

//-------------------------------------------
// FRSKY Para Wireless Traniner interface
// See https://github.com/ysoldak/HeadTracker

BTPara::BTPara() : BTFunction()
{
    bleconnected = false;
    info = new BLEService("180A");
    sysid = new BLECharacteristic("2A23", BLERead, 8);
    manufacturer = new BLECharacteristic("2A29", BLERead, 3);
    ieee = new BLECharacteristic("2A2A", BLERead, 14);
    pnpid = new BLECharacteristic("2A50", BLERead, 7);
    para = new BLEService("FFF0");
    fff1 = new BLEByteCharacteristic("FFF1", BLERead | BLEWrite);
    fff2 = new BLEByteCharacteristic("FFF2", BLERead);
    fff3 = new BLECharacteristic("FFF3", BLEWriteWithoutResponse, 32);
    fff5 = new BLECharacteristic("FFF5", BLERead, 32);
    fff6 = new BLECharacteristic("FFF6", BLEWriteWithoutResponse | BLENotify | BLEAutoSubscribe, 32);      
    
    serialWriteln("HT: Starting Para Bluetooth");
    
    BLE.setConnectable(true);
    BLE.setLocalName("Hello");
    
    info->addCharacteristic(*sysid);
    info->addCharacteristic(*manufacturer);
    info->addCharacteristic(*ieee);
    info->addCharacteristic(*pnpid);
    BLE.addService(*info);

    sysid->writeValue(sysid_data, 8);
    manufacturer->writeValue(m_data, 3);
    ieee->writeValue(ieee_data, 14);
    pnpid->writeValue(pnpid_data, 7);

    BLE.setAdvertisedService(*para);
    para->addCharacteristic(*fff1);
    para->addCharacteristic(*fff2);
    para->addCharacteristic(*fff3);
    para->addCharacteristic(*fff5);
    para->addCharacteristic(*fff6);

    BLE.addService(*para);
    fff1->writeValue(0x01);
    fff2->writeValue(0x02);

    BLE.advertise();

    // Save Local Address
    strcpy(_address,BLE.address().c_str());
}

BTPara::~BTPara()
{
    serialWriteln("HT: Stopping Para Bluetooth");

    // Disconnect
    BLE.disconnect();

    // Delete Service 1
    delete fff6;
    delete fff5;
    delete fff3;
    delete fff2;
    delete fff1;
    delete para;

    // Delete Service 2
    delete pnpid;
    delete ieee;
    delete manufacturer;
    delete sysid;
    delete info;    
}

void BTPara::execute()
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
     if(bleconnected) 
        sendTrainer();
}

void BTPara::sendTrainer() 
{
    uint8_t output[BLUETOOTH_LINE_LENGTH+1];
    int len;
    len = setTrainer(output);
    if(len > 0 && fff6 != nullptr)
        fff6->writeValue(output,len);
}