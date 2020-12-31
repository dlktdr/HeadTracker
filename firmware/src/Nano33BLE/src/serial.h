#ifndef SERIAL_H
#define SERIAL_H

#include "dataparser.h"

// ONLY use these serial write methods, they are buffered.
void serialWrite(arduino::String str);
void serialWrite(int val);
void serialWrite(char *data, int len);
void serialWrite(char const *data);
void serialWrite(char c);
void serialWrite(float f);
void serialWriteln(char const *data);
void serialWriteJSON(DynamicJsonDocument &json);


#endif