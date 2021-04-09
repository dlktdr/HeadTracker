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
    virtual uint16_t getChannel(int chno)=0;
    virtual int mode() {return BTDISABLE;}
    void setChannel(int channel, uint16_t value);
    void setChannelCount(int count);
    uint16_t chan_vals[16];

private:

    int num_chans;
};

#endif