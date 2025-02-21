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

#if defined(CONFIG_BT) && defined(CONFIG_BT_SETTINGS)

static struct HidReportInput1 btreport;
static uint16_t bthidchans[16];
static struct bt_conn *curconn = NULL;
static char _address[18] = "00:00:00:00:00:00";
static char _joystickname[] = "HT";
static uint8_t notify_hid_subscribed = false;
static uint8_t ctrl_point;
static int btinterval = 0;
char addr_str[50];

K_SEM_DEFINE(btJoystick_sem, 0, 1);

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

enum {
  HIDS_INPUT = 0x01,
  HIDS_OUTPUT = 0x02,
  HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
    .id = 0x01,
    .type = HIDS_INPUT,
};

static ssize_t read_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset)
{
  LOG_INF("BLE - Reading Info");
  return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                               uint16_t len, uint16_t offset)
{
  LOG_INF("BLE - Reading Report Map");
  return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_gamepad_report_desc, sizeof(hid_gamepad_report_desc));
}

static ssize_t read_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
  LOG_INF("BLE - Reading Report");
  return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
  LOG_INF("BLE - Notifications Requested");
  notify_hid_subscribed = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
  // On subscribe, give semaphore to initiate notifications
  if(notify_hid_subscribed)
    k_sem_give(&btJoystick_sem);
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
                           BT_GATT_PERM_READ_ENCRYPT, read_input_report, NULL, NULL),
    // Attribute 7
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report, NULL, &input),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &ctrl_point),
};

struct bt_gatt_service bthid_svc = BT_GATT_SERVICE(bthid_attr);

// static struct bt_le_adv_param my_joyparam = {
//     .id = BT_ID_DEFAULT,
//     .sid = 0,
//     .secondary_max_skip = 0,
//     .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
//     .interval_min = (BT_GAP_ADV_FAST_INT_MIN_2),
//     .interval_max = (BT_GAP_ADV_FAST_INT_MAX_2),
//     .peer = (NULL),
// };

static const struct bt_data hidad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
    BT_DATA(BT_DATA_NAME_COMPLETE, _joystickname, 2),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC4, 0x03),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
  if(bleconnected) {
    LOG_WRN("Bluetooth already connected");
    return;
  }

  bleconnected = true;

  curconn = bt_conn_ref(conn);
  if(curconn == NULL) {
    LOG_ERR("Failed to get connection reference");
    return;
  }
  struct bt_conn_info info;
  bt_conn_get_info(curconn, &info);

  bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
  LOG_INF("Connected to Address %s", addr_str);

  int rv = bt_conn_set_security(curconn, BT_SECURITY_L2);
	if (rv) {
		LOG_ERR("Failed to set security (err %d)", rv);
	}
}

// Joystick HID Report notify complete callback. This is called when the report has been sent.
// prevent bufffer from filling.

void btJoystickNotifyCompleteCB(struct bt_conn *conn, void *user_data)
{
  k_sem_give(&btJoystick_sem);
}

int startAdvertising()
{
  int err;
  err = bt_le_adv_start(BT_LE_ADV_CONN, hidad, ARRAY_SIZE(hidad), NULL, 0);
  if (err) {
    LOG_ERR("Advertising failed to start (err %d) (size %d)", err, ARRAY_SIZE(hidad));
    return err;
  } else {
    LOG_INF("Advertising successfully (size %d)", ARRAY_SIZE(hidad));
  }
  return err;
}


static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  LOG_WRN("Bluetooth disconnected (reason %d)", reason);

  if (curconn) {
    LOG_WRN("Cleaning up connection");
    bt_conn_unref(curconn);
  }
  curconn = NULL;
  bleconnected = false;
  notify_hid_subscribed = false;
  k_sem_reset(&btJoystick_sem);
}

void btjoyparamupdated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
  LOG_INF("Params Updated. Int:%d Lat:%d Timeout:%d", interval, latency, timeout);
  btinterval = interval;
}

static struct bt_conn_cb btj_conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_req = leparamrequested,
    .le_param_updated = btjoyparamupdated,
    .security_changed = securitychanged,
    .le_phy_updated = lephyupdated,
};


void joy_disconnect_conn_foreach(struct bt_conn *conn, void *data) {
  bt_conn_disconnect(conn, 0);
}

void BTJoystickStop()
{
  // Stop Advertising
  LOG_INF("Stopping Advertising");
  bt_le_adv_stop();

  // Kill current connection
  if (curconn) {
    LOG_INF("Disconnecting Active Connection");
    bt_conn_disconnect(curconn, 0);
    bt_conn_unref(curconn);
  }

  // Kill any other connections
  bt_conn_foreach(BT_CONN_TYPE_ALL, joy_disconnect_conn_foreach, NULL);
  curconn = NULL;
  notify_hid_subscribed = false;
  bleconnected = false;
  LOG_INF("Unregistering HID Service");
  bt_gatt_service_unregister(&bthid_svc);
  bt_conn_cb_register(NULL);
}

static bt_addr_le_t addrarry[CONFIG_BT_ID_MAX];
static size_t addrcnt = 1;

void BTJoystickStart()
{
  LOG_INF("Starting HID Joystick");
  bleconnected = false;
  btinterval = 0;
  notify_hid_subscribed = false;
  k_sem_reset(&btJoystick_sem);

  for (int i = 0; i < 16; i++) {
    bthidchans[i] = TrackerSettings::PPM_CENTER;
  }
  bt_set_name(_joystickname);

  LOG_INF("Registering HID Service");
  if(bt_gatt_service_register(&bthid_svc)) {
    LOG_ERR("Failed to register HID Service");
  }
  LOG_INF("Registering Callbacks");
  bt_conn_cb_register(&btj_conn_callbacks);

  // Discover BT Address
  bt_id_get(addrarry, &addrcnt);
  if (addrcnt > 0) bt_addr_le_to_str(&addrarry[0], _address, sizeof(_address));

  startAdvertising();
}

struct bt_gatt_notify_params ntfy_params = {
  .uuid = BT_UUID_HIDS_REPORT,
  .attr = NULL,
  .data = static_cast<void*>(&btreport.buttons[0]),
  .len = sizeof(struct HidReportInput1) - sizeof(HidReportInput1::ReportId),
  .func = btJoystickNotifyCompleteCB,
  .user_data = NULL,
};

int BTJoystickExecute()
{
  if (bleconnected) {
    clearLEDFlag(LED_BTSCANNING);
    setLEDFlag(LED_BTCONNECTED);

    // Send notifications if subsribed and buffer is free
    if (notify_hid_subscribed) {
      if (k_sem_take(&btJoystick_sem, K_NO_WAIT) == 0) {
        buildJoystickHIDReport(btreport, bthidchans);
        bt_gatt_notify_cb(curconn,&ntfy_params);

        /*
        // For debugging time between notifications
        static int mcount = 0;
        static int64_t mmic = millis64() + 1000;
        if (mmic < millis64()) {  // Every Second
          mmic = millis64() + 1000;
          LOG_INF("Notify Rate = %d", mcount);
          mcount = 0;
        }
        mcount++;
        */

      }
    }
  } else {
    // Scanning
    setLEDFlag(LED_BTSCANNING);
    clearLEDFlag(LED_BTCONNECTED);
  }
  return btinterval;
}

void BTJoystickSetChannel(int channel, const uint16_t value) { bthidchans[channel] = value; }
const char *BTJoystickGetAddress() { return _address; }

#else

// BT_SETTINGS not configured. Cannnot use an BLE HID without encryption. Device will not re-connect
// after connection loss without persistent settings.

void BTJoystickStop() {}
void BTJoystickStart() {}
int BTJoystickExecute() {return 0;}
void BTJoystickSetChannel(int channel, const uint16_t value) {}
const char *BTJoystickGetAddress() { return "BT_DISABLED"; }

#endif
