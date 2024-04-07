#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "log.h"

bool qmc5883Init();
bool qmc5883Read(float mag[3]);