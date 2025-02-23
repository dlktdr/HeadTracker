#pragma once

#include "ble.h"

extern uint16_t chanoverrides;

void btChannelsDecoded(uint16_t *channels);

void BTRmtStop();
void BTRmtStart();
void BTRmtExecute();
void BTRmtSetChannel(int channel, const uint16_t value);
uint16_t BTRmtGetChannel(int channel);
const char* BTRmtGetAddress();
void BTRmtSendButtonPress(bool longpress = false);
bool BTRmtGetConnected();
int8_t BTRmtGetRSSI();
