#pragma once

#include <drivers/i2c.h>
#include <zephyr.h>

#include "LSM6DS3/lsm6ds3tr-c_reg.h"

#define BOOT_TIME 30

int32_t platform_read_lsm6(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
int32_t platform_write_lsm6(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
int initailizeLSM6DS3(stmdev_ctx_t *dev_ctx);