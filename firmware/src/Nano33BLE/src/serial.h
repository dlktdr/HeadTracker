#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include "ArduinoJson.h"
#include "mbed.h"
#include "rtos.h"

using namespace mbed;
using namespace rtos;

static const int RX_BUF_SIZE=2000; // RX Buffer Size
static const int RX_BUFFERS=3; // Number of RX Buffers
static const int TX_BUF_SIZE=2000; // RX Buffer Size
static const int SERIAL_TX_MAX_PACKET = 60;

void serial_Init();
void serial_Thread(); // Transmits data in outgoing buffer.
void serialrx_Int();
void serialrx_Process();
char* getJSONBuffer();
int buffersFilled();


// ONLY use these serial write methods, they are buffered.
void serialWrite(arduino::String str);
void serialWrite(int val);
void serialWrite(char *data, int len);
void serialWrite(char const *data);
void serialWrite(char c);
void serialWrite(float f);
void serialWriteln(char const *data);
void serialWriteJSON(DynamicJsonDocument &json);

extern volatile bool JSONready;
extern volatile bool JSONfault;
extern volatile bool SerBufOverflow;
extern volatile bool BuffToSmall;

#endif