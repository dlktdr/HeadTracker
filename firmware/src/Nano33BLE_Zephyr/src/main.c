 /* Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <string.h>
#include <nrfx_rtc.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include "nano33ble.h"
#include "../include/arduino_nano_33_ble.h"

struct arduino_gpio_t S_gpios;

void main(void)
{
    start(); // Call Our C++ Main
}

static int board_internal_sensors_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	arduino_gpio_init(&S_gpios);

	NRF_PWM_Type * PWM[] = {
		NRF_PWM0, NRF_PWM1, NRF_PWM2, NRF_PWM3
	};

	for (unsigned int i = 0; i < (ARRAY_SIZE(PWM)); i++) {
		PWM[i]->ENABLE = 0;
		PWM[i]->PSEL.OUT[0] = 0xFFFFFFFFUL;
	}

	return 0;
}

SYS_INIT(board_internal_sensors_init, PRE_KERNEL_1, 32);
