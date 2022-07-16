#pragma once

#include "ble.h"

extern uint16_t chanoverrides;

void BTRmtStop();
void BTRmtStart();
void BTRmtExecute();
void BTRmtSetChannel(int channel, const uint16_t value);
uint16_t BTRmtGetChannel(int channel);
const char* BTRmtGetAddress();
void BTRmtSendButtonPress(bool longpress = false);
int8_t BTRmtGetRSSI();
