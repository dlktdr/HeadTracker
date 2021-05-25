#pragma once

#include <string>
#include <ArduinoJson.h>



void serial_Init();
void serial_Thread();

// ONLY use these serial write methods, they are buffered & thread safe
void serialWrite(std::string str);
void serialWrite(int val);
void serialWrite(const char *data, int len);
void serialWrite(const char *data);
void serialWrite(const char c);
void serialWriteln(const char *data);
void serialWriteJSON(DynamicJsonDocument &json);
void serialWriteF(const char *c, ...);
