#ifndef DATAPARSER_H
#define DATAPARSER_H

#include "trackersettings.h"

static const int UIRESPONSIVE_TIME = 10000; // 10Seconds without an ack data will stop;

void data_Thread();
void parseData(DynamicJsonDocument &json);

#endif