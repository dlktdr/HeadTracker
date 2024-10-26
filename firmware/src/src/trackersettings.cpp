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

#include "trackersettings.h"

#include <zephyr/logging/log.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "SBUS/sbus.h"
#include "base64.h"
#include "io.h"

#include "sense.h"
#include "soc_flash.h"

LOG_MODULE_REGISTER(trackersettings);

void TrackerSettings::setRollReversed(bool value)
{
  if (value)
    servoreverse |= ROLL_REVERSE_BIT;
  else
    servoreverse &= ~ROLL_REVERSE_BIT;
}

void TrackerSettings::setPanReversed(bool value)
{
  if (value)
    servoreverse |= PAN_REVERSE_BIT;
  else
    servoreverse &= ~PAN_REVERSE_BIT;
}

void TrackerSettings::setTiltReversed(bool value)
{
  if (value)
    servoreverse |= TILT_REVERSE_BIT;
  else
    servoreverse &= ~TILT_REVERSE_BIT;
}

bool TrackerSettings::isRollReversed() { return (servoreverse & ROLL_REVERSE_BIT); }

bool TrackerSettings::isTiltReversed() { return (servoreverse & TILT_REVERSE_BIT); }

bool TrackerSettings::isPanReversed() { return (servoreverse & PAN_REVERSE_BIT); }

void TrackerSettings::resetFusion() { reset_fusion(); }

// Saves current data to flash
void TrackerSettings::saveToEEPROM()
{
  uint8_t buffer[TX_RNGBUF_SIZE];

  k_mutex_lock(&data_mutex, K_FOREVER);
  json.clear();
  setJSONSettings(json);
  int len = serializeJson(json, buffer, TX_RNGBUF_SIZE);
  k_mutex_unlock(&data_mutex);

  if (socWriteFlash(buffer, len)) {
    LOG_ERR("Flash Write Failed size(%d)", len);
  } else {
    LOG_INF("Saved to Flash size(%d)", len);
  }
}

// Called on startup to read the data from Flash

void TrackerSettings::loadFromEEPROM()
{
  // Load Settings
  DeserializationError de;

  k_mutex_lock(&data_mutex, K_FOREVER);
  const uint8_t *memptr = socGetMMFlashPtr();
  if(memptr == NULL) {
    LOG_ERR("Unable to get flash memory pointer");
    k_mutex_unlock(&data_mutex);
    return;
  }
  de = deserializeJson(json, memptr);

  if (de != DeserializationError::Ok) LOG_ERR("Invalid JSON Data");

  if (json["UUID"] == 837727) {
    LOG_INF("Device has been freshly programmed, no data found");

  } else {
    LOG_INF("Loading settings from flash");
    loadJSONSettings(json);
  }
  k_mutex_unlock(&data_mutex);
}
