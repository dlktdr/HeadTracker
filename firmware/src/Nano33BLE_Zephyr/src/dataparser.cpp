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

#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
#include <zephyr.h>
#include <chrono>
#include "io.h"
#include "dataparser.h"
#include "serial.h"
#include "ucrc16lib.h"
#include "defines.h"

// Timers, Initially start timed out
extern volatile int64_t uiResponsive;
volatile bool uiconnected=false;

K_MUTEX_DEFINE(data_mutex);

static DynamicJsonDocument dtjson(JSON_BUF_SIZE);

// Handles all data transmission with the UI via Serial port
void data_Thread()
{
    while(1) {
        k_msleep(DATA_PERIOD);

        // Is the UI Still responsive?
        int64_t curtime = k_uptime_get();

        if(uiResponsive > curtime) {
            uiconnected = true;

            dtjson.clear();

            // If sense thread is writing, wait until complete
            k_mutex_lock(&data_mutex, K_FOREVER);
            trkset.setJSONData(dtjson);
            k_mutex_unlock(&data_mutex);

            dtjson["Cmd"] = "Data";
            serialWriteJSON(dtjson);

        }  else {
            uiconnected = false;
        }
    }
}

