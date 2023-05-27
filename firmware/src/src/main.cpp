/* Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <nrfx_rtc.h>
#include <pm/pm.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <usb/usb_device.h>
#include <zephyr.h>

#include "io.h"
#include "nano33ble.h"

extern "C" void main(void)
{
  start();  // Call Our C++ Main
}
