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
static char _address[18] = "00:00:00:00:00:00";
static struct bt_conn *pararmtconn = NULL;

struct bt_le_scan_param scnparams = {
    .type = BT_LE_SCAN_TYPE_ACTIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};


static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribefff6; // Channel Data
static struct bt_gatt_subscribe_params subscribeaff1; // Override Data

static bool contoheadboard = false;
uint32_t buttonhandle =0;
static const struct bt_gatt_attr *buttonattr=NULL;

struct bt_le_conn_param *rmtconparms = BT_LE_CONN_PARAM(BT_MIN_CONN_INTER_MASTER, BT_MAX_CONN_INTER_MASTER, 0, BT_CONN_LOST_TIME); // Faster Connection Interval

// Characteristic UUID
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);


// Read Override Parameters

uint8_t read_overrides(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
    serialWrite("HT: Read Override Data (0x");
    serialWriteHex((uint8_t*)data,2);
    serialWriteln(")");
    if(length == 2) {
        // Store Overrides
        memcpy(&chanoverrides, data, sizeof(chanoverrides));
    }

    return 0;
}

bt_gatt_read_params rparm = {
    .func = read_overrides,
};

/* Nofify function when bluetooth channel data has been sent
 */

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		return BT_GATT_ITER_CONTINUE;
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

/* Notify function when channel overrides have changed
 */

static uint8_t over_notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
    serialWrite("HT: BT Override Channels Changed (0x");
    serialWriteHex((uint8_t*)data,2);
    serialWriteln(")");
	if (!data) {
		return BT_GATT_ITER_CONTINUE;
	}

    // Store Overrides
    memcpy(&chanoverrides, data, sizeof(chanoverrides));

	return BT_GATT_ITER_CONTINUE;
}

// Discover Service Characteristics
static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
    static int ccclvl=0;

	if (!attr) {
		serialWrite("HT: Discover complete\r\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

#if defined(DEBUG)
    char str[30];
    bt_uuid_to_str(params->uuid,str,sizeof(str));
    serialWrite("Discovered UUID ");
    serialWrite(str);
    serialWriteln();

    serialWrite("Arribute Handle=");
    serialWrite(attr->handle);
    serialWriteln();
#endif

    // Found the FRSky FFF0 Service?
	if (!bt_uuid_cmp(discover_params.uuid, &frskyserv.uuid)) {
        serialWriteln("HT: Found FRSky Service 0xFFF0");
//-----------------------------------------------------------------------------------
// 0xFFF6
        // Setup next discovery
		memcpy(&uuid, &frskychar.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("HT: Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
	    }
    // Found the Frsky FFF6 Characteristic, Get the CCCD for it & subscribe
    } else if (!bt_uuid_cmp(discover_params.uuid, &frskychar.uuid)) {
        serialWriteln("HT: Found FRSky Caracteristic 0xFFF6");

//-----------------------------------------------------------------------------------
// 0xFFF6 (0x2902) - PARA CCC
        // Setup next discovery
        ccclvl = 0;
        memcpy(&uuid, &ccc.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribefff6.value_handle = attr->handle;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("HT: Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
		}

    // Found the FFF6 CCCD descriptor, subscribe to notifications
	} else if (!bt_uuid_cmp(discover_params.uuid, &ccc.uuid) && ccclvl == 0){
        serialWriteln("HT: Found FRSky CCC");

		subscribefff6.notify = notify_func;
		subscribefff6.value = BT_GATT_CCC_NOTIFY;
		subscribefff6.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribefff6);
		if (err && err != -EALREADY) {
            serialWrite("HT: Subscribe failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
		} else {
			serialWrite("HT: Subscribed to Frsky Data\r\n");
		}

//-----------------------------------------------------------------------------------
// 0xAFF1 - Override Channels
        // Setup next discovery (HT Override Characteristic)
        memcpy(&uuid, &htoverridech.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("HT: Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
	    }

	} else if (!bt_uuid_cmp(discover_params.uuid, &htoverridech.uuid)) {
        serialWriteln("HT: Found Override Characteristic");

//-----------------------------------------------------------------------------------
// 0xAFF1(0x2902) - Override's CCC

        // Setup next discovery, CCC for it
        ccclvl = 1;
        memcpy(&uuid, &ccc.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribeaff1.value_handle = attr->handle;
        err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("HT: Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
	    }

	} else if (!bt_uuid_cmp(discover_params.uuid, &ccc.uuid) && ccclvl == 1){
        serialWriteln("HT: Found Override CCC");

        subscribeaff1.notify = over_notify_func;
		subscribeaff1.value = BT_GATT_CCC_NOTIFY;
		subscribeaff1.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribeaff1);
		if (err && err != -EALREADY) {
            serialWrite("HT: Subscribe to overrides failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
		} else {
			serialWrite("HT: Subscribed to Overrides\r\n");
		}

//-----------------------------------------------------------------------------------
// 0xAFF2 - Override Button
        memcpy(&uuid, &btbutton.uuid, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        err = bt_gatt_discover(conn, &discover_params);
		if (err) {
           	serialWrite("HT: Discover failed (err ");
            serialWrite(err);
            serialWrite(")\r\n");
	    }

	// Found the HT Button Reset Characteristic
	} else if (!bt_uuid_cmp(discover_params.uuid, &btbutton.uuid)) {
        serialWriteln("HT: Found headboard connection");
        serialWriteln("HT: Enabling button indication forwarding");
        contoheadboard = true;
        buttonhandle = attr->handle;
        buttonattr = attr;


        serialWriteln("HT: Reading Overrides in Timeout");

        // Do an initial read of the BT override data
        rparm.handle_count = 1;
        rparm.single.handle = subscribeaff1.value_handle;
        rparm.single.offset = 0;

        bt_gatt_read( pararmtconn, &rparm );
	}

	return BT_GATT_ITER_STOP;
}

// Advertised Data Decoder

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

static struct bt_conn_le_phy_param phy_params = {
  .options = BT_CONN_LE_PHY_OPT_CODED_S8,
  .pref_tx_phy = BT_GAP_LE_PHY_CODED,
  .pref_rx_phy = BT_GAP_LE_PHY_CODED,
};

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

  bt_conn_info info;
  bt_conn_get_info(pararmtconn, &info);
  serialWrite("HT: PHY Connection Rx:");
  printPhy(info.le.phy->rx_phy);
  serialWrite(" Tx:");
  printPhy(info.le.phy->tx_phy);
  serialWriteln();

  serialWrite("HT: BT Connection Params Int:");
  serialWrite(info.le.interval);
  serialWrite(" Lat:");
  serialWrite(info.le.latency);
  serialWrite(" Timeout:");
  serialWrite(info.le.timeout);
  serialWriteln();


  serialWrite("HT: Requesting coded PHY - ");
  if(bt_conn_le_phy_update(pararmtconn, &phy_params))
    serialWriteln("FAILED");
  else
    serialWriteln("Success");

    // Start Discovery
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
    contoheadboard = false;
    buttonattr = NULL;
}

static struct bt_conn_cb rmtconn_callbacks = {
		.connected = rmtconnected,
		.disconnected = rmtdisconnected,
    .le_param_req = leparamrequested,
    .le_param_updated = leparamupdated,
    .le_phy_updated = lephyupdated
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

    bt_addr_le_t addrarry[CONFIG_BT_ID_MAX];
    size_t addrcnt=1;

    // Discover BT Address
    bt_id_get(addrarry, &addrcnt);
    if(addrcnt > 0) {
        bt_addr_le_to_str(&addrarry[0],_address,sizeof(_address));
    }

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
    return _address;
}

int8_t BTRmtGetRSSI()
{
    // *** Implement
    return -1;
}


static struct bt_gatt_write_params write_params;

char rstdata[1] = {'R'};

void btwrite_cb(bt_conn *conn, uint8_t err, bt_gatt_write_params *params){}

void BTRmtSendButtonPress(bool longpress)
{
    if(!buttonattr)
        return;

    if(longpress) {
        rstdata[0] = 'L';
        serialWriteln("HT: Sending Long Button Press to Head Board");
    }
    else {
        rstdata[0] = 'R';
        serialWriteln("HT: Sending Button Press to Head Board");
    }

    write_params.handle = sys_le16_to_cpu(buttonhandle);

    write_params.offset = 0U;
    write_params.data = (void *)rstdata;
    write_params.length = sys_le16_to_cpu(1);
    write_params.func = btwrite_cb;

    if(bt_gatt_write(pararmtconn, &write_params) < 0)
        serialWriteln("HT: Failed to send button press");
}



void BTRmtExecute()
{
    // All Async, Nothing Here
}


