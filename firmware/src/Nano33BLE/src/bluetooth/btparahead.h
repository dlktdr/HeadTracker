#ifndef BTPARAHEAD_H
#define BTPARAHEAD_H

#include <Arduino.h>
#include "ArduinoBLE.h"
#include "ble.h"


class BTParaHead : public BTFunction {
public:
    BTParaHead();
    ~BTParaHead();
    char *address() {return _address;}
    void execute();
    int mode() {return BTPARAHEAD;}
    bool isConnected() {return bleconnected;}
    uint16_t getChannel(int channel, bool &valid);
    void setChannel(int channel, uint16_t value);

private:
    void sendTrainer();
    bool bleconnected;

    BLELocalDevice *bledev;

    BLEService *para;
    BLECharacteristic *fff6;

    BLEService *rmbrd;
    BLECharacteristic *rbfff1;
    BLECharacteristic *rbfff2;
    BLECharacteristic *rbfff3;

    char _address[20];
};

#endif