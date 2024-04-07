#pragma once

#include <string>
#include "arduinojsonwrp.h"

#include "defines.h"

int serial_init();
void serial_Thread();

// ONLY use these serial write methods, they are buffered & thread safe
void serialWrite(const char *data, int len);
void serialWrite(const char *data);
int serialWriteF(const char *format, ...);
void serialWriteJSON(DynamicJsonDocument &json);

void JSON_Process(char *jsonbuf);

extern struct k_mutex data_mutex;
extern DynamicJsonDocument json;