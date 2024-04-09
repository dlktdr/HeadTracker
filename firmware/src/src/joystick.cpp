/*
 * Copyright (c) 2016-2018 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "io.h"
#include "joystick.h"

LOG_MODULE_REGISTER(joystick);

static const struct device *hdev;
static hidreport_s report;

const uint8_t hid_report_desc[] = {
    0x05, 0x01,  //     USAGE_PAGE (Generic Desktop)
    0x09, 0x05,  //     USAGE (Game Pad)
    0xa1, 0x01,  //     COLLECTION (Application)
    0xa1, 0x00,  //       COLLECTION (Physical)
    0x05, 0x09,  //         USAGE_PAGE (Button)
    0x19, 0x01,  //         USAGE_MINIMUM (Button 1)
    0x29, 0x10,  //         USAGE_MAXIMUM (Button 8)
    0x15, 0x00,  //         LOG_INFCAL_MINIMUM (0)
    0x25, 0x01,  //         LOG_INFCAL_MAXIMUM (1)
    0x95, 0x10,  //         REPORT_COUNT (8)
    0x75, 0x01,  //         REPORT_SIZE (1)
    0x81, 0x02,  //         INPUT (Data,Var,Abs)
    0x05, 0x01,        //         USAGE_PAGE (Generic Desktop)
    0x09, 0x30,        //         USAGE (X)
    0x09, 0x31,        //         USAGE (Y)
    0x09, 0x32,        //         USAGE (Z)
    0x09, 0x33,        //         USAGE (Rx)
    0x09, 0x34,        //         USAGE (Ry)
    0x09, 0x35,        //         USAGE (Rz)
    0x09, 0x36,        //         USAGE (Slider)
    0x09, 0x36,        //         USAGE (Slider)
    0x16, 0x00, 0x00,  //         LOG_INFCAL_MINIMUM (0)
    0x26, 0xFF, 0x03,  //         LOG_INFCAL_MAXIMUM (1024)
    0x75, 0x10,        //         REPORT_SIZE (16)
    0x95, 0x08,        //         REPORT_COUNT (8)
    0x81, 0x02,        //         INPUT (Data,Var,Abs)
    0xc0,              //       END_COLLECTION
    0xc0               //     END_COLLECTION
};

void buildJoystickHIDReport(hidreport_s &report, uint16_t chans[16])
{
  memcpy(report.channels, chans, sizeof(report.channels));

  report.but[0] = 0;
  report.but[1] = 0;

  for (int i = 0; i < 8; i++) {
    if (report.channels[i] == 0)  // If disabled, center it
      report.channels[i] = 1500;

    if (report.channels[i] >= JOYSTICK_BUTTON_HIGH) {
      report.but[0] |= 1 << (i * 2);
      report.but[1] |= 1 << ((i - 4) * 2);
    }

    if (report.channels[i] <= JOYSTICK_BUTTON_LOW) {
      report.but[0] |= 1 << ((i * 2) + 1);
      report.but[1] |= 1 << (((i - 4) * 2) + 1);
    }

    report.channels[i] -= 988;  // Shift from center so it's 0-1024
  }
}

void set_JoystickChannels(uint16_t chans[16])
{
  buildJoystickHIDReport(report, chans);
#if defined(CONFIG_USB_DEVICE_HID)
  hid_int_ep_write(hdev, (uint8_t *)&report, sizeof(report), NULL);
#endif
}

void joystick_init(void)
{
#ifndef CONFIG_USB_DEVICE_HID
  LOG_INF("USB HID not enabled");
  return;
#endif

  hdev = device_get_binding("HID_0");
  if (hdev == NULL) {
    LOG_ERR("Cannot bind to USB HID device");
    return;
  }

  usb_hid_register_device(hdev, hid_report_desc, sizeof(hid_report_desc), NULL);

  usb_hid_init(hdev);

  // USB enabled in serial.cpp
}
