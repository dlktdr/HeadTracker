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

#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "trackersettings.h"
#include "btparahead.h"
#include "serial.h"
#include "io.h"
#include "nano33ble.h"

static uint8_t ct[40];

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	/* TODO: Handle value */
}

static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len, uint16_t offset)
{
	char *value = (char*)attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(ct));
}

static ssize_t write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset,
			uint8_t flags)
{
	/*uint8_t *value = (char*)attr->user_data;

	if (offset + len > sizeof(ct)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	ct_update = 1U;
*/
	return len;
}

// Service UUID
static struct bt_uuid_128 btparaserv = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10,	0x00, 0x00, 0xf0, 0xff, 0x00, 0x00);

// Characteristic UUID
static struct bt_uuid_16 btparachar = BT_UUID_INIT_16(0xfff6);

BT_GATT_SERVICE_DEFINE(bt_srv,
    BT_GATT_PRIMARY_SERVICE(&btparaserv),
    BT_GATT_CHARACTERISTIC(&btparachar.uuid,
                           BT_GATT_CHRC_BROADCAST | BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
                           //BT_GATT_CHRC_BROADCAST| BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_ct,write_ct,ct),
    BT_GATT_CCC(ct_ccc_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    );

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(0xFFF0)),
    BT_DATA_BYTES(0x12, 0x00, 0x60, 0x00, 0x60),
    BT_DATA_BYTES(BT_DATA_TX_POWER, 0x00),
};

static struct bt_le_adv_param my_param = {
        .id = BT_ID_DEFAULT, \
        .sid = 0, \
        .secondary_max_skip = 0, \
        .options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME), \
        .interval_min = (BT_GAP_ADV_FAST_INT_MIN_2), \
        .interval_max = (BT_GAP_ADV_FAST_INT_MAX_2), \
        .peer = (NULL), \
    };

struct bt_conn *curconn = NULL;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		serialWriteF("HT: Bluetooth Connection failed (err 0x%02x)\r\n", err);
	} else {
		serialWriteln("HT: Bluetooth connected :)");
	}
    bt_le_adv_stop();

    curconn = conn;

    bleconnected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    serialWriteF("HT: Bluetooth disconnected (reason 0x%02x)\r\n", reason);

    // Start advertising
    int err = bt_le_adv_start(&my_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		serialWriteln("Advertising failed to start (err %d)");
		return;
	}

    curconn = NULL;


    bleconnected = false;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    serialWriteF("Passkey for %s: %06u\r\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	serialWriteF("Pairing cancelled: %s\r\n", addr);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

BTParaHead::BTParaHead() : BTFunction()
{
    bleconnected = false;

    serialWriteln("HT: Starting Head Para Bluetooth");


    // Start Advertising
    int err = bt_le_adv_start(&my_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		serialWriteln("Advertising failed to start (err %d)");
		return;
	}

    bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

    crc = 0;
    bufferIndex = 0;
}

BTParaHead::~BTParaHead()
{
    serialWriteln("HT: Stopping Head Para Bluetooth");
    // Stop Advertising
    bt_le_adv_stop();
    if(curconn)
        bt_conn_disconnect(curconn,0);
}

void BTParaHead::execute()
{
    static int ch1val=1000;
    static int ch2val=2000;
    if(bleconnected) {
        setChannel(0,ch1val);
        setChannel(1,ch2val);
        ch1val += 4;
        ch2val -= 4;
        if(ch1val> 2001)
            ch1val = 1000;

        if(ch2val < 999)
            ch2val = 2000;

        sendTrainer();
    }
}

void BTParaHead::sendTrainer()
{
    uint8_t output[BLUETOOTH_LINE_LENGTH+1];
    int len;
    len = setTrainer(output);
    bt_gatt_notify(NULL, &bt_srv.attrs[1], output, len);
}

// Part of setTrainer to calculate CRC
// From OpenTX

void BTParaHead::pushByte(uint8_t byte)
{
    crc ^= byte;
    if (byte == START_STOP || byte == BYTE_STUFF) {
        buffer[bufferIndex++] = BYTE_STUFF;
        byte ^= STUFF_MASK;
    }
    buffer[bufferIndex++] = byte;
}

void BTParaHead::setChannel(int channel, const uint16_t value)
{
    if(channel >= BT_CHANNELS)
        return;

    // If channel disabled, make a note for overriden characteristic
    // Actuall send it at center so PARA still works
    if(value == 0) {
        ovridech &= ~(1<<channel);
        chan_vals[channel] = TrackerSettings::PPM_CENTER;

    // Otherwise set the value and set that it is valid
    } else {
        ovridech |= 1<<channel;
        chan_vals[channel] = value;
    }
}

// Head BT does not return BT data
uint16_t BTParaHead::getChannel(int channel)
{
    return 0;
}

/* Builds Trainer Data
*     Returns the length of the encoded PPM + CRC
*     Data saved into addr pointer
*/
int BTParaHead::setTrainer(uint8_t *addr)
{
    // Allocate Channel Mappings, Set Default to all Center
    uint8_t * cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
    for (int channel=0; channel < BT_CHANNELS; channel+=2, cur+=3) {
        uint16_t channelValue1 = chan_vals[channel];
        uint16_t channelValue2 = chan_vals[channel+1];

        pushByte(channelValue1 & 0x00ff);
        pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
        pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
    }

    buffer[bufferIndex++] = crc;
    buffer[bufferIndex++] = START_STOP; // end byte

    // Copy data to array
    memcpy(addr,buffer,bufferIndex);

    return bufferIndex;
}