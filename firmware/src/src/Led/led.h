#pragma once

#include <cstdint>

#define LED_RUNNING 1<<0
#define LED_GYROCAL 1<<1
#define LED_BTCONNECTED 1<<2

void led_init();
void setLEDFlag(uint32_t ledMode);
void clearLEDFlag(uint32_t ledMode);
void clearAllFlags(uint32_t ledMode);
void led_Thread();
