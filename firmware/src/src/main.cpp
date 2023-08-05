/* Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <nrfx_rtc.h>
#include <zephyr/pm/pm.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/kernel.h>

#include "io.h"
#include "nano33ble.h"

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
static const struct device *clock0;
#endif

extern "C" int main(void)
{
#if defined(CONFIG_SOC_SERIES_NRF52X)
	CoreDebug->DEMCR = 0;
	NRF_CLOCK->TRACECONFIG = 0;

  // Force High Accuracy Clock
  clock0 = DEVICE_DT_GET(DT_NODELABEL(clock));
  if (clock0 == NULL) {
    printk("Failed to fetch clock");
  }
  clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);
#else
#error TEST
#endif
  start();  // Call Our C++ Main
  return 0;
}
