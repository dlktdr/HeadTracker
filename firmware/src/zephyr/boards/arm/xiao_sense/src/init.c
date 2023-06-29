/*
 * Copyright (c) 2020 Jefferson Lee.
 *   Modified by Cliff Blackburn 2023 for use with HeadTracker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)
static const struct device *clock0;

static int board_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CoreDebug->DEMCR = 0;
	NRF_CLOCK->TRACECONFIG = 0;

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
