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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "SBUS/sbus.h"
#include "base64.h"
#include "io.h"
#include "log.h"
#include "sense.h"
#include "soc_flash.h"

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

void TrackerSettings::pinsChanged()
{
  enum { PIN_PPMIN, PIN_PPMOUT, PIN_BUTRESET, PIN_SBUSIN1, PIN_SBUSIN2 };
  int pins[5]{-1, -1, -1, -1, -1};
  pins[PIN_PPMOUT] = getPpmOutPin();
  pins[PIN_PPMIN] = getPpmInPin();
  pins[PIN_BUTRESET] = getButtonPin();
  if (!getSbInInv()) {
    pins[PIN_SBUSIN1] = 5;
    pins[PIN_SBUSIN2] = 6;
  }

  // Loop through all possibilites checking for duplicates
  bool duplicates = false;
  for (int i = 0; i < 4; i++) {
    for (int y = i + 1; y < 5; y++) {
      if (pins[i] > 0 && pins[y] > 0 && pins[i] == pins[y]) {
        duplicates = true;
        break;
      }
    }
  }

  if (!duplicates) {
    PpmIn_setPin(-1);
    PpmOut_setPin(-1);
    PpmIn_setPin(getPpmInPin());
    PpmOut_setPin(getPpmOutPin());
  } else {
    LOGE("Cannot pick duplicate pins. Note: SBUS in uses D5+D6 when not inverted");
  }
}

// Saves current data to flash
void TrackerSettings::saveToEEPROM()
{
  char buffer[TX_RNGBUF_SIZE];

  k_mutex_lock(&data_mutex, K_FOREVER);
  setJSONSettings(json);
  int len = serializeJson(json, buffer, TX_RNGBUF_SIZE);
  k_mutex_unlock(&data_mutex);

  if (socWriteFlash(buffer, len)) {
    LOGE("Flash Write Failed");
  } else {
    LOGI("Saved to Flash");
  }
}

// Called on startup to read the data from Flash

void TrackerSettings::loadFromEEPROM()
{
  // Load Settings
  DeserializationError de;

  k_mutex_lock(&data_mutex, K_FOREVER);
  de = deserializeJson(json, get_flashSpace());

  if (de != DeserializationError::Ok) LOGE("Invalid JSON Data");

  if (json["UUID"] == 837727) {
    LOGI("Device has been freshly programmed, no data found");

  } else {
    LOGI("Loading settings from flash");
    loadJSONSettings(json);
  }
  k_mutex_unlock(&data_mutex);
}
