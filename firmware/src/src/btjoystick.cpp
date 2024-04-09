/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "btjoystick.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "defines.h"
#include "io.h"
#include "joystick.h"
#include "htmain.h"
#include "trackersettings.h"

LOG_MODULE_REGISTER(btjoystick);

#if defined(CONFIG_BT)

static hidreport_s report;
static uint16_t bthidchans[16];
static struct bt_conn *curconn = NULL;
static char _address[18] = "00:00:00:00:00:00";

enum {
  HIDS_REMOTE_WAKE = BIT(0),
  HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
  uint16_t version; /* version number of base USB HID Specification */
  uint8_t code;     /* country HID Device hardware is localized for. */
  uint8_t flags;
} __packed;

struct hids_report {
  uint8_t id;   /* report id */
  uint8_t type; /* report type */
} __packed;

static struct hids_info info = {
    .version = 0x0000,
    .code = 0x00,
    .flags = HIDS_NORMALLY_CONNECTABLE,
};

static struct bt_le_adv_param my_param = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
    .secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
    .interval_min = (BT_GAP_ADV_FAST_INT_MIN_2),
    .interval_max = (BT_GAP_ADV_FAST_INT_MAX_2),
    .peer = (NULL),
};

enum {
  HIDS_INPUT = 0x01,
  HIDS_OUTPUT = 0x02,
  HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
    .id = 0x00,
    .type = HIDS_INPUT,
};

static uint8_t simulate_input;
static uint8_t ctrl_point;

static ssize_t read_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset)
{
  return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                               uint16_t len, uint16_t offset)
{
  return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_report_desc, sizeof(hid_report_desc));
}

static ssize_t read_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
  return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(hidreport_s));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
  LOG_INF("BLE - Notifications Requested");
  simulate_input = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_input_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset)
{
  return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
  uint8_t *value = (uint8_t *)attr->user_data;

  if (offset + len > sizeof(ctrl_point)) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }

  memcpy(value + offset, buf, len);

  return len;
}

/* HID Service Declaration */
struct bt_gatt_attr bthid_attr[] = {
    // Attribute 0
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
    // Attribute 1,2
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_info, NULL,
                           &info),
    // Attribute 3,4
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                           read_report_map, NULL, NULL),
    // Attribute 5,6
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_input_report, NULL, NULL),
    // Attribute 7
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report, NULL, &input),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &ctrl_point),
};

struct bt_gatt_service bthid_svc = BT_GATT_SERVICE(bthid_attr);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC4, 0x03),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
  char addr[BT_ADDR_LE_STR_LEN];
  bleconnected = true;

  // Stop Advertising
  bt_le_adv_stop();
  curconn = bt_conn_ref(conn);

  if (err) {
    LOG_ERR("Failed to connect to %s (%u)\n", addr, err);
    return;
  }

  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);
  char addr_str[50];
  bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
  LOG_INF("Connected to Address %s", addr_str);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  LOG_WRN("Bluetooth disconnected (reason %d)", reason);

  // Start advertising
  int err = bt_le_adv_start(&my_param, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)", err);
    return;
  }

  if (curconn) bt_conn_unref(curconn);

  curconn = NULL;
  bleconnected = false;
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_req = leparamrequested,
    .le_param_updated = leparamupdated,
    .le_phy_updated = lephyupdated,
};

void BTJoystickStop()
{
  // Stop Advertising
  bt_le_adv_stop();

  // If connection open kill it
  if (curconn) {
    bt_conn_disconnect(curconn, 0);
    bt_conn_unref(curconn);
  }

  bt_gatt_service_unregister(&bthid_svc);
}

static bt_addr_le_t addrarry[CONFIG_BT_ID_MAX];
static size_t addrcnt = 1;

void BTJoystickStart()
{
  LOG_INF("Starting Bluetooth HID Joystick");
  bleconnected = false;

  for (int i = 0; i < 16; i++) {
    bthidchans[i] = TrackerSettings::PPM_CENTER;
  }

  bt_gatt_service_register(&bthid_svc);
  bt_conn_cb_register(&conn_callbacks);
  bt_set_name("HeadTracker Joystick");

  int err = bt_le_adv_start(&my_param, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)\n", err);
    return;
  }

    // Discover BT Address
  bt_id_get(addrarry, &addrcnt);
  if (addrcnt > 0) bt_addr_le_to_str(&addrarry[0], _address, sizeof(_address));


  LOG_INF("Advertising successfully started\n");
}

void BTJoystickExecute()
{
  if (bleconnected) {
    clearLEDFlag(LED_BTSCANNING);
    setLEDFlag(LED_BTCONNECTED);

    buildJoystickHIDReport(report, bthidchans);
    if (simulate_input)
      bt_gatt_notify(NULL, &bthid_svc.attrs[6], (void *)&report, sizeof(report));

  } else {
    // Scanning
    setLEDFlag(LED_BTSCANNING);
    clearLEDFlag(LED_BTCONNECTED);
  }
}

void BTJoystickSetChannel(int channel, const uint16_t value) { bthidchans[channel] = value; }

const char *BTJoystickGetAddress() { return _address; }

#endif
