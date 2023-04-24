/*
 * Copyright (c) 2016-2018 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <usb/class/usb_hid.h>
#include <usb/usb_device.h>
#include <zephyr.h>

#include "io.h"

static const struct device *hdev;
#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2
uint8_t report[4] = { 0x00 };

void set_JoystickChannels(uint16_t chans[16])
{
  int32_t xvalue = chans[0] - 1500; // Fixed to Channel 1, shift value to center
  int32_t yvalue = chans[1] - 1500; // Fixed to Channel 2

  report[MOUSE_BTN_REPORT_POS] = 0;
  report[MOUSE_X_REPORT_POS] = xvalue / 4; // Mouse is +/-127, HT range is 1024. so divide by 4.
  report[MOUSE_Y_REPORT_POS] = yvalue / 4;

#if defined(CONFIG_USB_DEVICE_HID)
  hid_int_ep_write(hdev, report, sizeof(report), NULL);
#endif
}

static const uint8_t hid_report_desc[] = {				\
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),			\
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),			\
	HID_COLLECTION(HID_COLLECTION_APPLICATION),		\
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),	\
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),	\
			/* Bits used for button signalling */	\
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),	\
			HID_USAGE_MIN8(1),			\
			HID_USAGE_MAX8(2),			\
			HID_LOGICAL_MIN8(0),			\
			HID_LOGICAL_MAX8(1),			\
			HID_REPORT_SIZE(1),			\
			HID_REPORT_COUNT(2),			\
			/* HID_INPUT (Data,Var,Abs) */		\
			HID_INPUT(0x02),			\
			/* Unused bits */			\
			HID_REPORT_SIZE(8 - 2),		\
			HID_REPORT_COUNT(1),			\
			/* HID_INPUT (Cnst,Ary,Abs) */		\
			HID_INPUT(1),				\
			/* X and Y axis, scroll */		\
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_WHEEL),	\
			HID_LOGICAL_MIN8((uint8_t)-127),			\
			HID_LOGICAL_MAX8(127),			\
			HID_REPORT_SIZE(8),			\
			HID_REPORT_COUNT(3),			\
			/* HID_INPUT (Data,Var,Rel) */		\
			HID_INPUT(0x06),			\
		HID_END_COLLECTION,				\
	HID_END_COLLECTION,					\
};

void joystick_init(void)
{
#ifndef CONFIG_USB_DEVICE_HID
  return;
#endif

  hdev = device_get_binding("HID_0");
  if (hdev == NULL) {
    return;
  }

  usb_hid_register_device(hdev, hid_report_desc, sizeof(hid_report_desc), NULL);

  usb_hid_init(hdev);

  // USB enabled in serial.cpp
}
