#ifndef SERIAL_H
#define SERIAL_H

#include "dataparser.h"

void serialWrite(arduino::String str);
void serialWriteln(char const *data);
void serialWrite(int val);
void serialWrite(char *data, int len);
void serialWrite(char const *data);
void serialWrite(char c);
void serialWriteF(float f); // fix
void serialWriteJSON(DynamicJsonDocument &json);

#endif