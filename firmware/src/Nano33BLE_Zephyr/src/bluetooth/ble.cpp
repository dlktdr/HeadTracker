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

#include "PPM/PPMOut.h"
#include "serial.h"
#include "io.h"
#include "nano33ble.h"
#include "dataparser.h"
#include "ble.h"

// Global Connected
bool bleconnected=false;

//-----------------------------------------------------

void bt_Thread()
{
    while(1) {
        k_msleep(BT_PERIOD);


        // Is BT Enabled
        BTFunction *bt = trkset.getBTFunc();
        if(bt != nullptr) {
            // Then call the execute function
            bt->execute();
            if(bt->isConnected())
                digitalWrite(LEDB,LOW);
            else
                digitalWrite(LEDB,HIGH);
        }
    }
}

void bt_Init()
{
	int err = bt_enable(NULL);
	if (err) {
		serialWrite("HT: Bluetooth init failed (err %d)");
        serialWrite(err);
        serialWriteln("");
		return;
	}

    serialWriteln("HT: Bluetooth initialized");
}

//
BTFunction::BTFunction()
{
    for(int i=0;i < BT_CHANNELS;i++) {
        chan_vals[i] = TrackerSettings::PPM_CENTER;
    }
}

BTFunction::~BTFunction()
{

}





