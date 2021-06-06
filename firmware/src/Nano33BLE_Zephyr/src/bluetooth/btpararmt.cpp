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
#include <sys/byteorder.h>

#include "serial.h"
#include "io.h"
#include "nano33ble.h"
#include "trackersettings.h"
#include "opentxbt.h"
#include "btpararmt.h"

static uint16_t chan_vals[BT_CHANNELS];
uint16_t chanoverrides=0xFFFF; // Default to all enabled

static void start_scan(void);
volatile bool isscanning=false;

static struct bt_conn *pararmtconn = NULL;

struct bt_le_scan_param scnparams = {
    .type = BT_LE_SCAN_TYPE_ACTIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

// UUID's
// CCCD UUID
struct bt_uuid_16 ccc = BT_UUID_INIT_16(0x2902);

// FrSky service and channel data characteristic
static struct bt_uuid_16 frskyserv = BT_UUID_INIT_16(0xFFF0);
static struct bt_uuid_16 frskychar = BT_UUID_INIT_16(0xFFF6);

// Head tracker specific data, remote button press, valid channels
static struct bt_uuid_16 htserv = BT_UUID_INIT_16(0xFFF1);
static struct bt_uuid_16 htvldchs = BT_UUID_INIT_16(0xFFF1);
static struct bt_uuid_16 htbutton = BT_UUID_INIT_16(0xFFF2);

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribefff6; // Channel Data
static struct bt_gatt_subscribe_params subscribefff1; //

struct bt_le_conn_param *rmtconparms = BT_LE_CONN_PARAM(16, 16, 0, 200); // Faster Connection Interval

// Characteristic UUID
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		printf("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

    // Simulate sending byte by byte like opentx uses, stores in global
    for(int i=0;i<length;i++) {
        processTrainerByte(((uint8_t *)data)[i]);
    }

    // Store all channels
    for(int i=0 ; i < BT_CHANNELS; i++) {
        // Only set the data on channels that are allowed to be overriden
            chan_vals[i]  = BtChannelsIn[i];
    }


	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		serialWrite("Discover complete\r\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

/*    char str[30];
    bt_uuid_to_str(discover_params.uuid,str,sizeof(str));

    serialWrite("Discovered UUID ");
    serialWrite(str);
    serialWriteln();
*/

    // Found the FRSky FFF0 Service?
	if (!bt_uuid_cmp(discover_params.uuid, &frskyserv.uuid)) {

        // Setup next discovery
		memcpy(&uuid, &frskychar.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
	    }

    // Found the Frsky FFF6 Characteristic, Get the CCCD for it & subscribe
    } else if (!bt_uuid_cmp(discover_params.uuid, &frskychar.uuid)) {

        // Setup next discovery
        memcpy(&uuid, &ccc.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribefff6.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
		}

    // Found the FFF6 CCCD descriptor, subscribe to notifications
	} else if (!bt_uuid_cmp(discover_params.uuid, &ccc.uuid)){
		subscribefff6.notify = notify_func;
		subscribefff6.value = BT_GATT_CCC_NOTIFY;
		subscribefff6.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribefff6);
		if (err && err != -EALREADY) {
            serialWrite("Subscribe failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
		} else {
			serialWrite("[SUBSCRIBED]\r\n");
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = (bt_addr_le_t *)user_data;
	int i;

	switch (data->type) {
    case BT_DATA_FLAGS:
        //serialWriteln("Flags Found");
        break;
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:   // *** DOESN'T WORK, MISSING DATA IN ADVERTISE???
        //serialWrite("Device Name ");
        //serialWriteln();
        //serialWrite((char*)data->data,data->data_len);
        break;
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			serialWriteln("HT: AD malformed");
			return true;
		}

        // Find all advertised UUID16 services
		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {

			uint16_t u16v;
            memcpy(&u16v, &data->data[i], sizeof(u16));

			if (u16v != 0xFFF0) {
				continue;
			}

	        char addrstr[BT_ADDR_LE_STR_LEN];
        	bt_addr_le_to_str(addr, addrstr, sizeof(addrstr));
            addrstr[17] = 0;

            /*serialWrite("HT: Has a FrSky Service on ");
            serialWrite(addrstr);
            serialWriteln();*/

            // Notify GUI that a valid BT device was discovered
            trkset.setDiscoveredBTHead(addrstr);

            // Scan only Mode, don't connect
            if(btscanonly)
                continue;

            // Only connect to a specific device?
            bool okaytocon = false;
            if(strlen(trkset.pairedBTAddress()) > 0) {
               if(strcmp(addrstr, trkset.pairedBTAddress()) == 0) {
                    okaytocon = true;
               }
               else {
                   serialWriteln("HT: FRSky device found. Not connecting. Incorrect Address");
               }
            } else
                okaytocon = true;


            if(!okaytocon)
                continue;

            // Stop Scanning
            if(isscanning)
                bt_le_scan_stop();

            serialWriteln("HT: Connecting to device...");

            struct bt_conn_le_create_param btconparm = {
                .options = (BT_CONN_LE_OPT_NONE),
                .interval = (0x0060),
                .window = (0x0060),
                .interval_coded = 0,
                .window_coded = 0,
                .timeout = 0, };

		    int	err = bt_conn_le_create(addr, &btconparm,
						rmtconparms, &pararmtconn);
			if (err) {
				serialWrite("HT: Create conn failed (Error ");
                serialWriteHex((uint8_t *)&err,1);
                serialWriteln(")");

                // Re-start Scanning
                start_scan();
                return true;
			}

			return false; // Stop parsing ad data
        }
        break;
    }

	return true; // Keep parsing ad data
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	if (pararmtconn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

    bt_data_parse(ad, eir_found, (void *)addr);
}

static void start_scan(void)
{
    int err = bt_le_scan_start(&scnparams, device_found);
	if (err) {
		serialWrite("HT: Scanning failed to start (err ");
        serialWrite(err);
        serialWriteln(")");
		return;
	}

    isscanning = true;
	serialWriteln("HT: Scanning successfully started");
}

static void rmtconnected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		serialWrite("HT: Failed to connect to ");
        serialWrite(addr);
        serialWrite(" (");
        serialWrite(err);
        serialWrite(")\r\n");

		bt_conn_unref(pararmtconn);
		pararmtconn = NULL;

		start_scan();
		return;
	}

	if (conn != pararmtconn) {
		return;
	}

	serialWrite("HT: Connected: ");
    serialWrite(addr);
    serialWriteln("");

    bleconnected = true;

        // Subscribe to FFF6 on Service FFF0
	if (conn == pararmtconn) {
		memcpy(&uuid, &frskyserv.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(pararmtconn, &discover_params);
		if (err) {
            serialWrite("Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
			return;
		}
	}
}

static void rmtdisconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	serialWrite("HT: Disconnected: ");
    serialWrite(addr);
    serialWriteln("");

	bt_conn_unref(pararmtconn);
	pararmtconn = NULL;

	start_scan();
    bleconnected = false;
}

static struct bt_conn_cb rmtconn_callbacks = {
		.connected = rmtconnected,
		.disconnected = rmtdisconnected,
};

void BTRmtStart()
{
    bleconnected = false;
    chanoverrides = 0xFFFF;

    // Reset all BT channels to disabled
    for(int i = 0; i < BT_CHANNELS; i++)
        chan_vals[i] = 0;

    serialWriteln("HT: Starting Remote Para Bluetooth");

    bt_conn_cb_register(&rmtconn_callbacks);

	start_scan();
}

// Close All Connections, Foreach Callback
void closeConnection(bt_conn *conn, void *data)
{
    bt_conn_disconnect(conn,BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

void BTRmtStop()
{
    bt_le_scan_stop();
    isscanning = false;

    serialWriteln("HT: Stopping Remote Para Bluetooth");

    // Close all Connections, Callback to above func
    bt_conn_foreach(BT_CONN_TYPE_ALL, closeConnection, NULL);

    // Reset all BT channels to center
    for(int i = 0; i <BT_CHANNELS; i++)
        chan_vals[i] = TrackerSettings::PPM_CENTER;

    bt_conn_cb_register(NULL);
}

void BTRmtSetChannel(int channel, const uint16_t value)
{
    // Only Receive, nothing here
}

uint16_t BTRmtGetChannel(int channel)
{
    if(channel >= 0 &&
       channel < BT_CHANNELS &&
       bleconnected == true &&
       (1 << channel) & chanoverrides)
            return chan_vals[channel];
    return 0;
}

const char * BTRmtGetAddress()
{
    return "";
}

void BTRmtSendButtonData(char bd)
{
    /*if(butpress)
        butpress.writeValue((uint8_t)bd);*/
}

void BTRmtExecute()
{
    // All Async.. Nothing Here
}


