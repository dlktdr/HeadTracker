#pragma once

#include "trackersettings.h"

extern volatile bool uiconnected;
extern struct k_mutex data_mutex;

void data_Thread();

