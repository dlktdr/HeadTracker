/*
 * Copyright (c) 2020 Jefferson Lee.
 *   Modified by Cliff Blackburn 2023 for use with HeadTracker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

/*
 * this method roughly follows the steps here:
 * https://github.com/arduino/ArduinoCore-nRF528x-mbedos/blob/6216632cc70271619ad43547c804dabb4afa4a00/variants/ARDUINO_NANO33BLE/variant.cpp#L136
 */

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)
static const struct device *clock0;

static int board_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CoreDebug->DEMCR = 0;
	NRF_CLOCK->TRACECONFIG = 0;

	NRF_PWM_Type * PWM[] = {
		NRF_PWM0, NRF_PWM1, NRF_PWM2, NRF_PWM3
	};

	for (unsigned int i = 0; i < (ARRAY_SIZE(PWM)); i++) {
		PWM[i]->ENABLE = 0;
		PWM[i]->PSEL.OUT[0] = 0xFFFFFFFFUL;
	}

  // Force High Accuracy Clock
  const char *clock_label = DT_LABEL(CLOCK_NODE);
  clock0 = DEVICE_DT_GET(DT_NODELABEL(clock));
  if (clock0 == NULL) {
    printk("Failed to fetch clock %s\n", clock_label);
  }
  clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);

	return 0;
}
SYS_INIT(board_init, PRE_KERNEL_1, 32);
