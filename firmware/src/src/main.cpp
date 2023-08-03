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

extern "C" int main(void)
{
  start();  // Call Our C++ Main
}
