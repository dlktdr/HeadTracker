#pragma once

#include <stdint.h>

uint16_t fmap(uint16_t x, float in_min, float in_max, float out_min, float out_max);
long map(long x, long in_min, long in_max, long out_min, long out_max);