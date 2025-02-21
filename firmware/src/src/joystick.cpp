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
#include "trackersettings.h"

LOG_MODULE_REGISTER(joystick);

static const struct device *hdev;
static struct HidReportInput1 usb_hid_report;

void buildJoystickHIDReport(struct HidReportInput1 &report, uint16_t chans[16])
{
  for(unsigned int i = 0; i < sizeof(struct HidReportInput1); i++)
    ((uint8_t *)&report)[i] = 0;
  report.ReportId = 1;

  for (int i = 0; i < 8; i++) {
    int curchval = (int)chans[i];
    if (curchval == 0)  // If disabled (equals zero), center it
      curchval = TrackerSettings::PPM_CENTER;

    curchval -= TrackerSettings::PPM_CENTER;  // Shift the center to zero

    if(curchval > 511) curchval = 511;
    if(curchval < -511) curchval = -511;

    if (curchval >= JOYSTICK_BUTTON_HIGH) {
      if(i < 4)
        report.buttons[0] |= (1 << (i * 2) & 0xFF);
      else
        report.buttons[1] |= (1 << ((i - 4) * 2) & 0xFF);
    }

    if (curchval <= JOYSTICK_BUTTON_LOW) {
      if(i < 4)
        report.buttons[0] |= (1 << (i * 2 + 1) & 0xFF);
      else
        report.buttons[1] |= (1 << ((i - 4) * 2 + 1) & 0xFF);
    }
    switch(i) {
      case 0:
        report.ch1 = curchval;
        break;
      case 1:
        report.ch2 = curchval;
        break;
      case 2:
        report.ch3 = curchval;
        break;
      case 3:
        report.ch4 = curchval;
        break;
      case 4:
        report.ch5 = curchval;
        break;
      case 5:
        report.ch6 = curchval;
        break;
      case 6:
        report.ch7 = curchval;
        break;
      case 7:
        report.ch8 = curchval;
        break;
    }
  }

}

void set_JoystickChannels(uint16_t chans[16])
{
#if defined(CONFIG_USB_DEVICE_HID)
  if(hdev) {
    buildJoystickHIDReport(usb_hid_report, chans);
    hid_int_ep_write(hdev, (uint8_t *)&usb_hid_report, sizeof(usb_hid_report), NULL);
  }
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

  usb_hid_register_device(hdev, hid_gamepad_report_desc, sizeof(hid_gamepad_report_desc), NULL);

  usb_hid_init(hdev);
  // USB enabled in serial.cpp
}
