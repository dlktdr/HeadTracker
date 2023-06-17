/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2023 Cliff Blackburn
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

#pragma once

#include <bluetooth/conn.h>

#include "bluetooth/btparahead.h"
#include "bluetooth/btpararmt.h"
#include "bluetooth/btjoystick.h"
#include "defines.h"

#define LEN_BLUETOOTH_ADDR 16
#define MAX_BLUETOOTH_DISTANT_ADDR 6
#define BLUETOOTH_LINE_LENGTH 32

extern volatile bool bleconnected;
extern volatile bool btscanonly;
extern struct bt_uuid_16 ccc;
extern struct bt_uuid_16 frskyserv;
extern struct bt_uuid_16 frskychar;
extern struct bt_uuid_16 htoverridech;
extern struct bt_uuid_16 btbutton;
extern struct bt_uuid_16 jsonuuid;

/* TODO: Work me in?, instead of using the switch statements
typedef struct {
  void (*start)();
  void (*stop)();
  void (*exec)();
  void (*setChannel)(int channel, uint16_t value);
  uint16_t (*getChannel)(int channel);
  const char* (*getAddress)();
  bool (*getConnected)();
} bluetoothMode;

bluetoothMode bluetoothModes[] = {
  // Disabled Mode
  {
   .start = nullptr,
   .stop = nullptr,
   .exec = nullptr,
   .setChannel = nullptr,
   .getChannel = nullptr,
   .getAddress = nullptr,
   .getConnected = nullptr
  },
  // Para Head
  {
   .start = BTHeadStart,
   .stop = BTHeadStop,
   .exec = BTHeadExecute,
   .setChannel = BTHeadSetChannel,
   .getChannel = nullptr,
   .getAddress = BTHeadGetAddress,
   .getConnected = BTHeadGetConnected
  },

  // Para Remote
  {
   .start = BTRmtStart,
   .stop = BTRmtStop,
   .exec = BTRmtExecute,
   .setChannel = nullptr,
   .getChannel = BTRmtGetChannel,
   .getAddress = BTRmtGetAddress,
   .getConnected = BTRmtGetConnected
  },
  // Scan Only

  // Bt Joystick

  // TODO Finish me
};
*/

typedef enum { BTDISABLE = 0, BTPARAHEAD, BTPARARMT, BTSCANONLY, BTJOYSTICK } btmodet;

void bt_Thread();
void bt_init();
void BTSetMode(btmodet mode);
btmodet BTGetMode();
uint16_t BTGetChannel(int chno);
void BTSetChannel(int channel, const uint16_t value);
bool BTGetConnected();
const char *BTGetAddress();
int8_t BTGetRSSI();

bool leparamrequested(struct bt_conn *conn, struct bt_le_conn_param *param);
void leparamupdated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout);
void securitychanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
void lephyupdated(struct bt_conn *conn, struct bt_conn_le_phy_info *param);
const char *printPhy(int);
