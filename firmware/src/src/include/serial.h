#pragma once

#include <string>
#define ARDUINOJSON_USE_DOUBLE 0
#include <include/arduinojsonwrp.h>

#include "defines.h"

void serial_init();
void serial_Thread();

// ONLY use these serial write methods, they are buffered & thread safe
void serialWrite(const char *data, int len);
void serialWrite(const char *data);
int serialWriteF(const char *format, ...);
void serialWriteJSON(DynamicJsonDocument &json);

void JSON_Process(char *jsonbuf);

extern struct k_mutex data_mutex;
extern DynamicJsonDocument json;