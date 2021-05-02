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
    uint16_t getChannel(int channel);
    void setChannel(int channel, const uint16_t value);

private:
    void sendTrainer();
    int setTrainer(uint8_t *addr);
    void pushByte(uint8_t byte);
    static constexpr uint8_t START_STOP = 0x7E;
    static constexpr uint8_t BYTE_STUFF = 0x7D;
    static constexpr uint8_t STUFF_MASK = 0x20;
    uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
    uint8_t bufferIndex;
    uint8_t crc;

    bool bleconnected;

    BLELocalDevice *bledev;

    BLEService *para;
    BLECharacteristic *fff6;

    BLEService *rmbrd;
    BLECharacteristic *rbfff1;
    BLECharacteristic *rbfff2;
    BLECharacteristic *rbfff3;

    char _address[20];
    uint16_t ovridech = 0;
};

#endif