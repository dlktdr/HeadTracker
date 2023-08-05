/*
 * Copyright (c) 2020 Jefferson Lee.
 *   Modified by Cliff Blackburn 2023 for use with HeadTracker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

/*
 * this method roughly follows the steps here:
 * https://github.com/arduino/ArduinoCore-nRF528x-mbedos/blob/6216632cc70271619ad43547c804dabb4afa4a00/variants/ARDUINO_NANO33BLE/variant.cpp#L136
 */

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)
static const struct device *clock0;

static int board_init(void)
{
	CoreDebug->DEMCR = 0;
	NRF_CLOCK->TRACECONFIG = 0;

  // Force High Accuracy Clock
  clock0 = DEVICE_DT_GET(DT_NODELABEL(clock));
  if (clock0 == NULL) {
    printk("Failed to fetch clock");
  }
  clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);

	return 0;
}

SYS_INIT(board_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
