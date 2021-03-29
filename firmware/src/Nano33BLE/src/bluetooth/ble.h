#ifndef BLE_H
#define BLE_H

#include "Arduino.h"
#include "ArduinoBLE.h"

void bt_Thread();
void bt_Init();

#define LEN_BLUETOOTH_ADDR              16
#define MAX_BLUETOOTH_DISTANT_ADDR      6
#define BLUETOOTH_LINE_LENGTH           32
#define BT_CHANNELS 8

enum {
    BTDISABLE=0,
    BTPARAHEAD,
    BTPARARMT
};

class BTFunction {
public:
    BTFunction();
    virtual ~BTFunction();
    virtual char *address()=0;
    virtual void execute()=0;
    virtual bool isConnected()=0;
    virtual uint16_t getChannel(int chno, bool &valid)=0;
    virtual int mode() {return BTDISABLE;}
    void setChannel(int channel, uint16_t value);
    void setChannelCount(int count);
    uint16_t chan_vals[16];

protected:
    int setTrainer(uint8_t *addr);
    void pushByte(uint8_t byte);

private:
    static constexpr uint8_t START_STOP = 0x7E;
    static constexpr uint8_t BYTE_STUFF = 0x7D;
    static constexpr uint8_t STUFF_MASK = 0x20;
    uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
    uint8_t bufferIndex;
    uint8_t crc;
    int num_chans;
};

#endif