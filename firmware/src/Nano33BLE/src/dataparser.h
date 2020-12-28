#ifndef SERIAL_HH
#define SERIAL_HH

#include "trackersettings.h"
#include <mbed.h>

static const int RT_BUF_SIZE=1000; // RX Buffer Size
static const int TX_BUF_SIZE=1000; // RX Buffer Size
static const int DATA_THREAD_PERIOD = 10; // 10ms Period, Data Thread, Apx.
static const int DATA_SEND_TIME=150; // Data Update Speed in ms
static const int UIRESPONSIVE_TIME=10000; // 10Seconds without an ack data will stop;

void data_Thread();
void serialrx_Int();
void serialWrite(char const *data);
void serialWrite(char c);
void serialWriteln(char const *data);
void serialWrite(char *data,int len);
void serialWrite(int val);
void serialWrite(arduino::String str);
void serialWriteJSON(DynamicJsonDocument &json);

extern CircularBuffer<char, TX_BUF_SIZE> serout;
extern Mutex serWriteMutex;

using namespace mbed;

#endif