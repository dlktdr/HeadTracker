#pragma once
#include "ble.h"

void BTJoystickStop();
void BTJoystickStart();
int BTJoystickExecute();
const char *BTJoystickGetAddress();
void BTJoystickSetChannel(int channel, const uint16_t value);
