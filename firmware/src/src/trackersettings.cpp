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
#include "io.h"
#include "log.h"
#include "sense.h"
#include "soc_flash.h"
#include "base64.h"


void TrackerSettings::setRollReversed(bool value)
{
  if (value)
    servorev |= ROLL_REVERSE_BIT;
  else
    servorev &= (ROLL_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setPanReversed(bool value)
{
  if (value)
    servorev |= PAN_REVERSE_BIT;
  else
    servorev &= (PAN_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setTiltReversed(bool value)
{
  if (value)
    servorev |= TILT_REVERSE_BIT;
  else
    servorev &= (TILT_REVERSE_BIT ^ 0xFFFF);
}

bool TrackerSettings::isRollReversed() { return (servorev & ROLL_REVERSE_BIT); }

bool TrackerSettings::isTiltReversed() { return (servorev & TILT_REVERSE_BIT); }

bool TrackerSettings::isPanReversed() { return (servorev & PAN_REVERSE_BIT); }

void TrackerSettings::resetFusion() {
  reset_fusion();
}

void TrackerSettings::pinsChanged() {
  // TODO. Check for duplicates, disable all pins, then re-enable

}

/*void TrackerSettings::setButtonPin(int value)
{
  if (value > 1 && value < 13) {
    if (buttonpin > 0) pinMode(D_TO_32X_PIN(buttonpin), GPIO_INPUT);  // Disable old button pin
    pinMode(D_TO_32X_PIN(value), INPUT_PULLUP);                       // Button as Input
    buttonpin = value;                                                // Save

    // Disable the Button Pin
  } else if (value < 0) {
    if (buttonpin > 0) pinMode(D_TO_32X_PIN(buttonpin), GPIO_INPUT);  // Disable old button pin
    buttonpin = -1;
  }
}

void TrackerSettings::setPpmOutPin(int value)
{
  if ((value > 1 && value < 13) || value == -1) {
    PpmOut_setPin(value);
    ppmoutpin = value;
  }
}

void TrackerSettings::setPpmInPin(int value)
{
  if ((value > 1 && value < 13) || value == -1) {
    PpmIn_setPin(value);
    ppminpin = value;
  }
}*/

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

