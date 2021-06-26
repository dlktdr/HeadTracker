/*
 * Copyright (c) 2016-2018 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include "io.h"

static enum usb_dc_status_code usb_status;
static uint16_t channeldata[8];
static void send_report(struct k_work *work);

void set_JoystickChannels(uint16_t chans[16])
{
#ifndef CONFIG_USB_DEVICE_HID
    return;
#endif
    memcpy(channeldata, chans, sizeof(channeldata));
    send_report(NULL);
}

#define REPORT_TIMEOUT K_SECONDS(2)
#define HID_EP_BUSY_FLAG	0
#define REPORT_ID_1		0x01
#define REPORT_PERIOD		K_SECONDS(2)

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

/* Some HID sample Report Descriptor */
/*static const uint8_t hid_report_desc[] = {
        0x05, 0x01,                     // Usage Page (Generic Desktop)
        0x15, 0x00,                     // Logical Minimum (0)
        0x09, 0x04,                     // Usage (Joystick)
        0xA1, 0x01,                     // Collection (Application)
            0xA1, 0x00,                     // Collection ()
                0x16, 0x00, 0x00,               // Logical Minimum (0)
                0x26, 0xFF, 0x03,               // Logical Maximum (1023)
                0x75, 0x10,                     // Report Size (16)
                0x95, 0x08,                       // Report Count ()
                0x09, 0x30,                     // Usage (X)
                0x09, 0x31,                     // Usage (Y)
                0x09, 0x32,                     // Usage (Z)
                0x09, 0x33,                     // Usage (Rx)
                0x09, 0x34,                     // Usage (Ry)
                0x09, 0x35,                     // Usage (Rz)
                0x09, 0x36,                     // Usage (Slider)
                0x09, 0x36,                     // Usage (Slider)
                0x81, 0x82,                     // Input (variable,absolute)
            0xC0,                           // End Collection
        0xC0                            // End Collection

             // value in () is the number of bytes.  These bytes follow the comma, least significant byte first
            // see USBHID_Types.h for more info
             USAGE_PAGE(1), 0x01,           // Generic Desktop
             LOGICAL_MINIMUM(1), 0x00,      // Logical_Minimum (0)
             USAGE(1), 0x04,                // Usage (Joystick)
             COLLECTION(1), 0x01,           // Application

// 6 Axes of Joystick
               USAGE_PAGE(1), 0x01,            // Generic Desktop
               USAGE(1), 0x01,                 // Usage (Pointer)
               COLLECTION(1), 0x00,            // Physical
                 USAGE(1), 0x30,                 // X
                 USAGE(1), 0x31,                 // Y
                 USAGE(1), 0x32,                 // Z
                 USAGE(1), 0x33,                 // RX
                 USAGE(1), 0x34,                 // RY
                 USAGE(1), 0x35,                 // RZ
                 LOGICAL_MINIMUM(2), 0x00, 0x80, // -32768 (using 2's complement)
                 LOGICAL_MAXIMUM(2), 0xff, 0x7f, // 32767 (0x7fff, least significant byte first)
                 REPORT_SIZE(1), 0x10,  // REPORT_SIZE describes the number of bits in this element (16, in this case)
                 REPORT_COUNT(1), 0x06,
                 INPUT(1), 0x02,                 // Data, Variable, Absolute
               END_COLLECTION(0),

#if (HAT4 == 1)
// 4 Position Hat Switch
               USAGE(1), 0x39,                 // Usage (Hat switch)
               LOGICAL_MINIMUM(1), 0x00,       // 0
               LOGICAL_MAXIMUM(1), 0x03,       // 3
               PHYSICAL_MINIMUM(1), 0x00,      // Physical_Minimum (0)
               PHYSICAL_MAXIMUM(2), 0x0E, 0x01, // Physical_Maximum (270)
               UNIT(1), 0x14,                  // Unit (Eng Rot:Angular Pos)
               REPORT_SIZE(1), 0x04,
               REPORT_COUNT(1), 0x01,
               INPUT(1), 0x02,                 // Data, Variable, Absolute
#endif
#if (HAT8 == 1)
// 8 Position Hat Switch
               USAGE(1), 0x39,                 // Usage (Hat switch)
               LOGICAL_MINIMUM(1), 0x00,       // 0
               LOGICAL_MAXIMUM(1), 0x07,       // 7
               PHYSICAL_MINIMUM(1), 0x00,      // Physical_Minimum (0)
               PHYSICAL_MAXIMUM(2), 0x3B, 0x01, // Physical_Maximum (45 deg x (max of)7 = 315)
               UNIT(1), 0x14,                  // Unit (Eng Rot:Angular Pos)
               REPORT_SIZE(1), 0x04,
               REPORT_COUNT(1), 0x01,
               INPUT(1), 0x02,                 // Data, Variable, Absolute
#endif

// Padding 4 bits
               REPORT_SIZE(1), 0x01,
               REPORT_COUNT(1), 0x04,
               INPUT(1), 0x01,                 // Constant


#if (BUTTONS4 == 1)
// 4 Buttons
               USAGE_PAGE(1), 0x09,            // Buttons
               USAGE_MINIMUM(1), 0x01,         // 1
               USAGE_MAXIMUM(1), 0x04,         // 4
               LOGICAL_MINIMUM(1), 0x00,       // 0
               LOGICAL_MAXIMUM(1), 0x01,       // 1
               REPORT_SIZE(1), 0x01,
               REPORT_COUNT(1), 0x04,
               UNIT_EXPONENT(1), 0x00,         // Unit_Exponent (0)
               UNIT(1), 0x00,                  // Unit (None)
               INPUT(1), 0x02,                 // Data, Variable, Absolute

// Padding 4 bits
               REPORT_SIZE(1), 0x01,
               REPORT_COUNT(1), 0x04,
               INPUT(1), 0x01,                 // Constant

#endif
#if (BUTTONS8 == 1)
// 8 Buttons
               USAGE_PAGE(1), 0x09,            // Buttons
               USAGE_MINIMUM(1), 0x01,         // 1
               USAGE_MAXIMUM(1), 0x08,         // 8
               LOGICAL_MINIMUM(1), 0x00,       // 0
               LOGICAL_MAXIMUM(1), 0x01,       // 1
               REPORT_SIZE(1), 0x01,
               REPORT_COUNT(1), 0x08,
               UNIT_EXPONENT(1), 0x00,         // Unit_Exponent (0)
               UNIT(1), 0x00,                  // Unit (None)
               INPUT(1), 0x02,                 // Data, Variable, Absolute
#endif

#if (BUTTONS32 == 1)
// 32 Buttons
               USAGE_PAGE(1), 0x09,            // Buttons
               USAGE_MINIMUM(1), 0x01,         // 1
               USAGE_MAXIMUM(1), 0x20,         // 32
               LOGICAL_MINIMUM(1), 0x00,       // 0
               LOGICAL_MAXIMUM(1), 0x01,       // 1
               REPORT_SIZE(1), 0x01,
               REPORT_COUNT(1), 0x20,
               UNIT_EXPONENT(1), 0x00,         // Unit_Exponent (0)
               UNIT(1), 0x00,                  // Unit (None)
               INPUT(1), 0x02,                 // Data, Variable, Absolute
#endif

             END_COLLECTION(0)
};
*/



typedef struct {
} packetchdata;

static const uint8_t hid_report_desc[] = "";//HID_MOUSE_REPORT_DESC(2);
static volatile uint8_t status[4];

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(main);

static const struct device *hdev;
static struct k_work report_send;
static ATOMIC_DEFINE(hid_ep_in_busy, 1);

uint8_t report[4] = { 0x00 };

static void send_report(struct k_work *work)
{

  /*  struct {
        uint8_t data[64];
        uint16_t length;
    }  report;*/

    int chnout[8];

    for(int i=0; i <8 ; i++) {
        chnout[i] = channeldata[i];
        if(chnout[i] == 0) // If disabled, center it
            chnout[i] = 1500;
        chnout[i] -= 1500; // Shift from center so it's 0-1024
        chnout[i] *= 1<<5; // Scale to 15bits, already at 10bits..
    }

    // Pack all the data as 10bits
/*    union {
        struct {
        unsigned ch1 : 10;
        unsigned ch2 : 10;
        unsigned ch3 : 10;
        unsigned ch4 : 10;
        unsigned ch5 : 10;
        unsigned ch6 : 10;
        unsigned ch7 : 10;
        unsigned ch8 : 10;
        };
        uint8_t packed[20];
    } chunion;


    chunion.ch1 = channeldata[0];
    chunion.ch2 = channeldata[1];
    chunion.ch3 = channeldata[2];
    chunion.ch4 = channeldata[3];
    chunion.ch5 = channeldata[4];
    chunion.ch6 = channeldata[5];
    chunion.ch7 = channeldata[6];
    chunion.ch8 = channeldata[7];

       // Fill the report according to the Joystick Descriptor
   report.data[0] = chnout[0] & 0xff;
   report.data[1] = (chnout[0]>> 8) & 0xff;
   report.data[2] = chnout[1] & 0xff;
   report.data[3] = (chnout[1] >> 8) & 0xff;
   report.data[4] = chnout[2] & 0xff;
   report.data[5] = (chnout[2] >> 8) & 0xff;
   report.data[6] = chnout[3] & 0xff;
   report.data[7] = (chnout[3] >> 8) & 0xff;
   report.data[8] = chnout[4] & 0xff;
   report.data[9] = (chnout[4] >> 8) & 0xff;
   report.data[10] = chnout[5] & 0xff;
   report.data[11] = (chnout[5] >> 8) & 0xff;

#if (BUTTONS4 == 1)
//Hat and 4 Buttons
//   report.data[4] = ((_buttons & 0x0f) << 4) | (_hat & 0x0f) ;
//   report.length = 5;

//Use 4 bit padding for hat4 or hat8
   report.data[12] = (0 & 0x0f) ;

//Use 4 bit padding for buttons
   report.data[13] = (0 & 0x0f) ;
   report.length = 8;
#endif

	int ret;
    uint32_t wrote;

	//ret = hid_int_ep_write(hdev, report.data, report.length, &wrote);*/


    //gpio_pin_set()


    report[MOUSE_BTN_REPORT_POS] = status[MOUSE_BTN_REPORT_POS];
    report[MOUSE_X_REPORT_POS] = 10;
    status[MOUSE_X_REPORT_POS] = 0U;
    report[MOUSE_Y_REPORT_POS] = 1;
    status[MOUSE_Y_REPORT_POS] = 0U;
    hid_int_ep_write(hdev, report, sizeof(report), NULL);
}

static void int_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	if (!atomic_test_and_clear_bit(hid_ep_in_busy, HID_EP_BUSY_FLAG)) {
		LOG_WRN("IN endpoint callback without preceding buffer write");
	}
}

/*
 * On Idle callback is available here as an example even if actual use is
 * very limited. In contrast to report_event_handler(),
 * report value is not incremented here.
 */
static void on_idle_cb(const struct device *dev, uint16_t report_id)
{
	LOG_DBG("On idle callback");
	k_work_submit(&report_send);
}

static void protocol_cb(const struct device *dev, uint8_t protocol)
{
	LOG_INF("New protocol: %s", protocol == HID_PROTOCOL_BOOT ?
		"boot" : "report");
}

static const struct hid_ops ops = {
    .protocol_change = protocol_cb,
    .on_idle = on_idle_cb,
	.int_in_ready = int_in_ready_cb,
};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
    usb_status = status;
	/*switch (status) {
	case USB_DC_RESET:
		configured = false;
		break;
	case USB_DC_CONFIGURED:
		if (!configured) {
			int_in_ready_cb(hdev);
			configured = true;
		}
		break;
	case USB_DC_SOF:
		break;
	default:
		LOG_DBG("status %u unhandled", status);
		break;
	}*/
}

void joystick_init(void)
{
#ifndef CONFIG_USB_DEVICE_HID
    return;
#endif

    int ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}
}

static int composite_pre_init(const struct device *dev)
{
#ifndef CONFIG_USB_DEVICE_HID
    return 0;
#endif

	hdev = device_get_binding("HID_0");
	if (hdev == NULL) {
		return -ENODEV;
	}

	usb_hid_register_device(hdev, hid_report_desc, sizeof(hid_report_desc),
				NULL);

//	atomic_set_bit(hid_ep_in_busy, HID_EP_BUSY_FLAG);
	//k_timer_start(&event_timer, REPORT_PERIOD, REPORT_PERIOD);

    int a  = usb_hid_init(hdev);
	return a;
}

SYS_INIT(composite_pre_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);