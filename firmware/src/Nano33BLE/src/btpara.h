#ifndef BTPARA_H
#define BTPARA_H

#include "arduino.h"
#include "ArduinoBLE.h"
#include "ble.h"

class BTPara : public BTFunction {
public:    
    BTPara();
    ~BTPara();
    char *address() {return _address;}
    void execute();
    int mode() {return BTPARA;}
    bool isConnected() {return bleconnected;}

private:
    void sendTrainer();
    bool bleconnected;
    uint8_t sysid_data[8] = { 0xF1, 0x63, 0x1B, 0xB0, 0x6F, 0x80, 0x28, 0xFE };
    uint8_t m_data[3] = { 0x41, 0x70, 0x70 };
    uint8_t ieee_data[14] = { 0xFE, 0x00, 0x65, 0x78, 0x70, 0x65, 0x72, 0x69, 0x6D, 0x65, 0x6E, 0x74, 0x61, 0x6C };
    uint8_t pnpid_data[7] = { 0x01, 0x0D, 0x00, 0x00, 0x00, 0x10, 0x01 };

    BLELocalDevice *bledev;
    BLEService *info;
    BLECharacteristic *sysid;
    BLECharacteristic *manufacturer;
    BLECharacteristic *ieee;
    BLECharacteristic *pnpid;

    BLEService *para;
    BLEByteCharacteristic *fff1;
    BLEByteCharacteristic *fff2;
    BLECharacteristic *fff3;
    BLECharacteristic *fff5;
    BLECharacteristic *fff6;

    char _address[20];
};

#endif