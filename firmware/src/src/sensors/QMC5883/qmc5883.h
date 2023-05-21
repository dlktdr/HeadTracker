#pragma once

#include <device.h>
#include <drivers/i2c.h>
#include <zephyr.h>

#include "log.h"

bool qmc5883Init();
bool qmc5883Read(float mag[3]);