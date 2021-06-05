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

static struct bt_conn *pararmtconn = NULL;

struct bt_le_scan_param scnparams = {
    .type = BT_LE_SCAN_TYPE_PASSIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

struct bt_le_conn_param *rmtconparms = BT_LE_CONN_PARAM(6, 8, 0, 100); // Faster Connection Interval

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = (bt_addr_le_t *)user_data;
	int i;

	/*serialWrite("[AD]: ");
    serialWrite(data->type);
    serialWrite(" data_len ");
    serialWrite(data->data_len);
    serialWriteln("");*/

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
                    serialWriteln("HT: Not connecting to found device, not the paired device");
               }
            } else
                okaytocon = true;


            if(!okaytocon)
                continue;

            serialWriteln("HT: Connecting to device...");

			int err = bt_le_scan_stop();
			if (err) {
				serialWrite("HT: Stop LE scan failed (err ");
                serialWrite(err);
                serialWriteln(")");
				continue;
			}

            struct bt_conn_le_create_param btconparm = {
                .options = (BT_CONN_LE_OPT_NONE),
                .interval = (0x0060),
                .window = (0x0060),
                .interval_coded = 0,
                .window_coded = 0,
                .timeout = 0, };

			err = bt_conn_le_create(addr, &btconparm,
						rmtconparms, &pararmtconn);
			if (err) {
				serialWrite("HT: Create conn failed (err ");
                serialWrite(err);
                serialWriteln(")");

                // Re-start Scanning
                start_scan();
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
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(&scnparams, device_found);
	if (err) {
		serialWrite("HT: Scanning failed to start (err ");
        serialWrite(err);
        serialWriteln(")");
		return;
	}

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


void BTRmtStop()
{
    bt_le_scan_stop();

    if(pararmtconn) {
        bt_conn_disconnect(pararmtconn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }

    // Reset all BT channels to center
    for(int i = 0; i <BT_CHANNELS; i++)
        chan_vals[i] = TrackerSettings::PPM_CENTER;

    serialWriteln("HT: Stopping Remote Para Bluetooth");

    bt_conn_cb_register(NULL);
}

void BTRmtSetChannel(int channel, const uint16_t value)
{

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
     /*// Start Scan for PARA Slaves
    if(!BLE.connected() && !scanning) {
        serialWriteln("BRMT: Starting Scan");
        BLE.scan(1);
        scanning = true;
        bleconnected = false;
    }

    // If scanning see if there is a BLE device available
    if(!BLE.connected() && scanning) {
        bool fault = false;
        peripheral = BLE.available();
        if(peripheral) {
#ifdef DEBUG
            if (peripheral.address() == "") {
                serialWrite("BRMT:  <no advertised address> ");
            } else {
                serialWrite("BRMT: Advertised Device Address: ");
                serialWrite(peripheral.address());
            }
            if (peripheral.localName() == "") {
                serialWrite(" <no advertised local name> ");
            } else {
                serialWrite(" Local Name: ");
                serialWrite(peripheral.localName());
            }

            if (peripheral.advertisedServiceUuid() == "") {
                serialWrite(" <no advertised service UUID> ");
            } else {
                serialWrite(" Advertised Service UUID ");
                serialWrite(peripheral.advertisedServiceUuid());
            }
            serialWriteln("");
#endif
            if(peripheral.localName() == "Hello" &&
               peripheral.advertisedServiceUuid() == "fff0") {

#ifdef DEBUG
                serialWriteln("BRMT: Found a PARA device");
                serialWriteln("BRMT: Stopping scan");
#endif
                trkset.setDiscoveredBTHead(peripheral.address().c_str());
                BLE.stopScan();
                scanning = false;
#ifdef DEBUG
                serialWriteln("BRMT: Connecting...");
#endif
                if(peripheral.connect()) {
                    serialWriteln("BRMT: Connected");
                    ThisThread::sleep_for(std::chrono::milliseconds(150));
#ifdef DEBUG
                    serialWriteln("BRMT: Discovering Attributes");
#endif
                    if(peripheral.discoverAttributes()) {
#ifdef DEBUG
                        serialWriteln("BRMT: Discovered Attributes");
#endif
                        fff6 = peripheral.service("fff0").characteristic("fff6");
                        if(fff6) {
#ifdef DEBUG
                            serialWriteln("BRMT: Attaching Event Handler");
#endif
                            fff6.setEventHandler(BLEWritten, fff6Written);  // Call this function on data received
                            serialWriteln("BRMT: Found channel data characteristic");
#ifdef DEBUG
                            serialWriteln("BRMT: Subscribing...");
#endif
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(fff6.subscribe()) {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribed to data!");
#endif
                            } else {
                                serialWriteln("BRMT: Subscribe to data failed");
                                fault = true;
                            }
                        } else  {
#ifdef DEBUG
                            serialWriteln("BRMT: Couldn't find characteristic");
#endif
                            fault = true;
                        }
                        // If this is a Headtracker it may have a reset center option
                        butpress = peripheral.service("fff1").characteristic("fff2");
                        if(butpress) {
#ifdef DEBUG
                            serialWriteln("BRMT: Tracker has ability to remote reset center");
#endif
                        }
                        overridech = peripheral.service("fff1").characteristic("fff1");
                        if(overridech) {
                            overridech.setEventHandler(BLEWritten, overrideWritten);  // Call this function on data received
                            // Initial read of overridden channels
                            overridech.readValue(chanoverrides);
#ifdef DEBUG
                            serialWriteln("BRMT: Tracker has sent the channels it wants overriden");
#endif
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(overridech.subscribe()) {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribed to channel overrides!");
#endif
                            } else {
#ifdef DEBUG
                                serialWriteln("BRMT: Subscribe to override Failed");
#endif
                                fault = true;
                            }
                        } else
                            chanoverrides = 0xFFFF;
                    } else {
                        serialWriteln("BRMT: Attribute Discovery Failed");
                        fault = true;
                        }
                } else {
                    serialWriteln("BRMT: Couldn't connect to Para Slave, Rescanning");
                    fault = true;
                }
            }
        }
        // On any faults, disconnect and start scanning again
        if(fault) {
            peripheral.disconnect();
            BLE.scan(1);
            scanning = true;
        }
    }

    // Connected
    if(BLE.connected()) {
        // Check how long we have been connected but haven't received any data
        // if longer than timeout, disconnect.
        uint32_t wdtime = std::chrono::duration_cast<std::chrono::milliseconds>(watchdog.elapsed_time()).count();
        if(wdtime > WATCHDOG_TIMEOUT) {
            serialWriteln("BRMT: ***WATCHDOG.. Forcing disconnect. No data received");
            BLE.disconnect();
            bleconnected = false;
        }
    // Not Connected
    } else {
        bleconnected = false;
        watchdog.reset();
    }

    BLE.poll();
}

void overrideWritten(BLEDevice central, BLECharacteristic characteristic)
{
    characteristic.readValue(chanoverrides);
}

// Called when Radio Outputs new Data
void fff6Written(BLEDevice central, BLECharacteristic characteristic) {
    // Got Data Must Be Connected
    bleconnected = true;

    // Got some data clear the watchdog timer
    watchdog.reset();

    uint8_t buffer1[BLUETOOTH_LINE_LENGTH+1];
    int len = characteristic.readValue(buffer1,32);

    // Simulate sending byte by byte like opentx uses, stores in global
    for(int i=0;i<len;i++) {
        processTrainerByte(buffer1[i]);
    }

    // Store all channels
    for(int i=0 ; i < BT_CHANNELS; i++) {
        // Only set the data on channels that are allowed to be overriden
        if(BTParaInst) {
            BTParaInst->chan_vals[i]  = BtChannelsIn[i];
        }
    }

#ifdef DEBUG
    Serial.print("OR: ");
    printHex(chanoverrides);
    Serial.print("|");
    for(int i=0;i<CHANNEL_COUNT;i++) {
        Serial.print("Ch");Serial.print(i+1);Serial.print(":");Serial.print(ppmChannels[i]);Serial.print(" ");
    }
    Serial.println("");
#endif*/
}


