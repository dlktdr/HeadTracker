#pragma once
#include "ble.h"

void BTHeadStop();
void BTHeadStart();
void BTHeadExecute();
void BTHeadSetChannel(int channel, const uint16_t value);
uint16_t BTHeadGetChannel(int channel);
const char* BTHeadGetAddress();
int8_t BTHeadGetRSSI();
