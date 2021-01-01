#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include "ArduinoJson.h"
#include "mbed.h"
#include "rtos.h"

using namespace mbed;
using namespace rtos;

static const int RT_BUF_SIZE=1000; // RX Buffer Size
static const int TX_BUF_SIZE=1000; // RX Buffer Size
static const int SERIAL_THREAD_PERIOD = 15; 
extern char jsondatabuf[RT_BUF_SIZE];

extern void serial_Thread();
extern void serialrx_Int();

// ONLY use these serial write methods, they are buffered.
void serialWrite(arduino::String str);
void serialWrite(int val);
void serialWrite(char *data, int len);
void serialWrite(char const *data);
void serialWrite(char c);
void serialWrite(float f);
void serialWriteln(char const *data);
void serialWriteJSON(DynamicJsonDocument &json);

extern CircularBuffer<char, TX_BUF_SIZE> serout;
extern CircularBuffer<char, TX_BUF_SIZE> serin;
extern Mutex serWriteMutex;

extern volatile bool JSONready;
extern volatile bool JSONfault;
extern volatile bool SerBufOverflow;
extern volatile bool BuffToSmall;

#endif