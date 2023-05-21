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

#include "ble.h"

extern uint16_t chanoverrides;

void BTRmtStop();
void BTRmtStart();
void BTRmtExecute();
void BTRmtSetChannel(int channel, const uint16_t value);
uint16_t BTRmtGetChannel(int channel);
const char* BTRmtGetAddress();
void BTRmtSendButtonPress(bool longpress = false);
int8_t BTRmtGetRSSI();
