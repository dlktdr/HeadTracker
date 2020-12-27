#ifndef SERIAL_H
#define SERIAL_H

#include "trackersettings.h"
#include "rtos.h"

void data_Thread();
void serialrx_Int();
void serialWrite(char const *data);
void serialWrite(char c);
void serialWriteln(char const *data);
void serialWrite(char *data,int len);
void serialWrite(int val);
void serialWrite(arduino::String str);
void serialWriteJSON(DynamicJsonDocument &json);

using namespace mbed;

#endif