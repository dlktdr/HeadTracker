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
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>

#include "serial.h"
#include "io.h"
#include "nano33ble.h"
#include "ble.h"

// Globals
volatile bool bleconnected=false;
volatile bool btscanonly = false;
btmodet curmode = BTDISABLE;

// UUID's
struct bt_uuid_16 ccc = BT_UUID_INIT_16(0x2902);
struct bt_uuid_16 frskyserv = BT_UUID_INIT_16(0xFFF0);
struct bt_uuid_16 frskychar = BT_UUID_INIT_16(0xFFF6);
struct bt_uuid_16 htoverridech = BT_UUID_INIT_16(0xAFF1);
struct bt_uuid_16 btbutton = BT_UUID_INIT_16(0xAFF2);
struct bt_uuid_16 jsonuuid = BT_UUID_INIT_16(0xAFF3);

// Switching modes, don't execute
volatile bool btThreadRun = false;

void bt_init()
{
    int err = bt_enable(NULL);
	if (err) {
		serialWrite("HT: Bluetooth init failed (err %d)");
        serialWrite(err);
        serialWriteln("");
		return;
	}

    serialWriteln("HT: Bluetooth initialized");
    btThreadRun = true;
}

void bt_Thread()
{
  int64_t usduration=0;
  while(1) {
    usduration = micros64();

    if(!btThreadRun) {
      rt_sleep_ms(10);
      continue;
    }

    switch(curmode) {
    case BTPARAHEAD:
      BTHeadExecute();
      break;
    case BTPARARMT:
      BTRmtExecute();
      break;
    case BTSCANONLY:
      break;
    default:
      break;
    }

    if(bleconnected)
      setLEDFlag(LED_BTCONNECTED);
    else
      clearLEDFlag(LED_BTCONNECTED);

    // Adjust sleep for a more accurate period
    usduration = micros64() - usduration;
    if(BT_PERIOD - usduration < BT_PERIOD * 0.7) {  // Took a long time. Will crash if sleep is too short
      rt_sleep_us(BT_PERIOD);
    } else {
      rt_sleep_us(BT_PERIOD - usduration);
    }
  }
}

void BTSetMode(btmodet mode)
{
    // Requested same mode, just return
    if(mode == curmode)
        return;

    btThreadRun = false;

    // Shut Down
    switch(curmode) {
    case BTPARAHEAD:
        BTHeadStop();
        break;
    case BTPARARMT:
        BTRmtStop();
        btscanonly = false;
        break;
    case BTSCANONLY:
        BTRmtStop();
        btscanonly = false;
        break;
    default:
        break;
    }

    // Start Up
    switch(mode) {
    case BTPARAHEAD:
        BTHeadStart();
        break;
    case BTPARARMT:
        btscanonly = false;
        BTRmtStart();
        break;
    case BTSCANONLY:
        btscanonly = true;
        BTRmtStart();
    default:
        break;
    }

    btThreadRun = true;

    curmode = mode;
}

btmodet BTGetMode()
{
    return curmode;
}

bool BTGetConnected()
{
    if(curmode == BTDISABLE)
        return false;
    return bleconnected;
}

uint16_t BTGetChannel(int chno)
{
    switch(curmode) {
    case BTPARAHEAD:
        return BTHeadGetChannel(chno);
    case BTPARARMT:
        return BTRmtGetChannel(chno);
    case BTSCANONLY:
        return 0;
    default:
        break;
    }

    return 0;
}

void BTSetChannel(int channel, const uint16_t value)
{
    switch(curmode) {
    case BTPARAHEAD:
        BTHeadSetChannel(channel, value);
        break;
    case BTPARARMT:
        BTRmtSetChannel(channel, value);
        break;
    default:
        break;
    }
}

const char *BTGetAddress()
{
    switch(curmode) {
    case BTPARAHEAD:
        return BTHeadGetAddress();
    case BTPARARMT:
    case BTSCANONLY:
        return BTRmtGetAddress();
    default:
        break;
    }
    return "BT_DISABLED";
}

int8_t BTGetRSSI()
{
    switch(curmode) {
    case BTPARAHEAD:
        return BTHeadGetRSSI();
    case BTPARARMT:
        return BTRmtGetRSSI();
        break;
    default:
        break;
    }

    return -1;
}

bool leparamrequested(struct bt_conn *conn, struct bt_le_conn_param *param)
{
  serialWrite("HT: Bluetooth Params Request. IntMax:");
  serialWrite(param->interval_max);
  serialWrite(" IntMin:");
  serialWrite(param->interval_min);
  serialWrite(" Lat:");
  serialWrite(param->latency);
  serialWrite(" Timout:");
  serialWrite(param->timeout);
  serialWriteln();
  return true;
}

void leparamupdated(struct bt_conn *conn,
                           uint16_t interval,
				                   uint16_t latency,
                           uint16_t timeout)
{
  serialWrite("HT: Bluetooth Params Updated. Int:");
  serialWrite(interval);
  serialWrite(" Lat:");
  serialWrite(latency);
  serialWrite(" Timeout:");
  serialWrite(timeout);
  serialWriteln();
}

void securitychanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
  serialWrite("HT: Bluetooth Security Changed. Lvl:");
  serialWrite(level);
  serialWrite(" Err:");
  serialWrite(err);
  serialWriteln();
}

void printPhy(int phy)
{
  switch(phy) {
    case BT_GAP_LE_PHY_NONE:
      serialWrite("None");
      break;
    case BT_GAP_LE_PHY_1M:
      serialWrite("1M");
      break;
    case BT_GAP_LE_PHY_2M:
      serialWrite("2M");
      break;
    case BT_GAP_LE_PHY_CODED:
      serialWrite("Coded");
      break;
  }
}

void lephyupdated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
  serialWrite("HT: Bluetooth PHY Updated. RxPHY:");
  printPhy(param->rx_phy);
  serialWrite(" TxPHY:");
  printPhy(param->tx_phy);
  serialWriteln();
}
