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
  ppmininvert = inv;
  PpmIn_setInverted(inv);
}

//---------------------------------
// Bluetooth Settings

int TrackerSettings::blueToothMode() const { return btmode; }

void TrackerSettings::setBlueToothMode(int mode)
{
  if (mode < BTDISABLE || mode > BTSCANONLY) return;

  BTSetMode(btmodet(mode));
  btmode = mode;
}

void TrackerSettings::setBLEValues(uint16_t vals[BT_CHANNELS])
{
  memcpy(btch, vals, sizeof(uint16_t) * BT_CHANNELS);
}

void TrackerSettings::setSenseboard(bool sense) { isSense = sense; }

void TrackerSettings::setPairedBTAddress(const char *ha)
{
  // set to "" for pair to first available
  strncpy(btpairedaddress, ha, sizeof(btpairedaddress));
}

const char *TrackerSettings::pairedBTAddress() { return btpairedaddress; }

//----------------------------------------------------------------
// Orentation

void TrackerSettings::setOrientation(int rx, int ry, int rz)
{
  rotx = rx;
  roty = ry;
  rotz = rz;
  reset_fusion();  // Cause imu to reset
}

void TrackerSettings::orientRotations(float rot[3])
{
  rot[0] = rotx;
  rot[1] = roty;
  rot[2] = rotz;
}

//----------------------------------------------------------------------------------------
// Data sent the PC, for calibration and info

void TrackerSettings::setBLEAddress(const char *addr) { strcpy(btaddr, addr); }

void TrackerSettings::setDiscoveredBTHead(const char *addr) { strcpy(btrmt, addr); }

void TrackerSettings::setPPMInValues(uint16_t vals[16])
{
  for (int i = 0; i < 16; i++) ppmch[i] = vals[i];
}

void TrackerSettings::setSBUSValues(uint16_t vals[16])
{
  for (int i = 0; i < 16; i++) sbusch[i] = vals[i];
}

void TrackerSettings::setChannelOutValues(uint16_t vals[16])
{
  for (int i = 0; i < 16; i++) chout[i] = vals[i];
}

void TrackerSettings::setRawGyro(float x, float y, float z)
{
  gyrox = x;
  gyroy = y;
  gyroz = z;
}

void TrackerSettings::setRawAccel(float x, float y, float z)
{
  accx = x;
  accy = y;
  accz = z;
}

void TrackerSettings::setRawMag(float x, float y, float z)
{
  magx = x;
  magy = y;
  magz = z;
}

void TrackerSettings::setOffGyro(float x, float y, float z)
{
  off_gyrox = x;
  off_gyroy = y;
  off_gyroz = z;
}

void TrackerSettings::setOffAccel(float x, float y, float z)
{
  off_accx = x;
  off_accy = y;
  off_accz = z;
}

void TrackerSettings::setOffMag(float x, float y, float z)
{
  off_magx = x;
  off_magy = y;
  off_magz = z;
}

void TrackerSettings::setRawOrient(float t, float r, float p)
{
  tilt = t;
  roll = r;
  pan = p;
}

void TrackerSettings::setOffOrient(float t, float r, float p)
{
  tiltoff = t;
  rolloff = r;
  panoff = p;
}

void TrackerSettings::setPPMOut(uint16_t t, uint16_t r, uint16_t p)
{
  tiltout = t;
  rollout = r;
  panout = p;
}

void TrackerSettings::setQuaternion(float q[4]) { memcpy(quat, q, sizeof(float) * 4); }

//--------------------------------------------------------------------------------------
// Send and receive the data from PC
// Takes the JSON and loads the settings into the local class

void TrackerSettings::loadJSONSettings(DynamicJsonDocument &json)
{
  // Channels
  JsonVariant v, v1, v2;

  // Channels
  v = json["rllch"];
  if (!v.isNull()) setRollCh(v);
  v = json["tltch"];
  if (!v.isNull()) setTiltCh(v);
  v = json["panch"];
  if (!v.isNull()) setPanCh(v);
  v = json["alertch"];
  if (!v.isNull()) setAlertCh(v);

  // Servo Reversed
  v = json["servoreverse"];
  if (!v.isNull()) setServoreverse((uint8_t)v);

  // Roll
  v = json["rll_min"];
  if (!v.isNull()) setRll_min(v);
  v = json["rll_max"];
  if (!v.isNull()) setRll_max(v);
  v = json["rll_gain"];
  if (!v.isNull()) setRll_gain(v);
  v = json["rll_cnt"];
  if (!v.isNull()) setRll_cnt(v);

  // Tilt
  v = json["tlt_min"];
  if (!v.isNull()) setTlt_min(v);
  v = json["tlt_max"];
  if (!v.isNull()) setTlt_max(v);
  v = json["tlt_gain"];
  if (!v.isNull()) setTlt_gain(v);
  v = json["tlt_cnt"];
  if (!v.isNull()) setTlt_cnt(v);

  // Pan
  v = json["pan_min"];
  if (!v.isNull()) setPan_min(v);
  v = json["pan_max"];
  if (!v.isNull()) setPan_max(v);
  v = json["pan_gain"];
  if (!v.isNull()) setPan_gain(v);
  v = json["pan_cnt"];
  if (!v.isNull()) setPan_cnt(v);

  // Misc Gains
  v = json["lppan"];
  if (!v.isNull()) setLPPan(v);
  v = json["lptiltroll"];
  if (!v.isNull()) setLPTiltRoll(v);

  // Bluetooth
  v = json["btmode"];
  if (!v.isNull()) setBlueToothMode(v);
  v = json["btpair"];
  if (!v.isNull()) setPairedBTAddress(v);

  // Orientation
  v = json["rotx"];
  if (!v.isNull()) setOrientation(v, roty, rotz);
  v = json["roty"];
  if (!v.isNull()) setOrientation(rotx, v, rotz);
  v = json["rotz"];
  if (!v.isNull()) setOrientation(rotx, roty, v);

  // Reset On Wave
  v = json["rstonwave"];
  if (!v.isNull()) setResetOnWave(v);

  // Button and Pins
  bool setpins = false;
  int bp = buttonpin;
  int ppmi = ppminpin;
  int ppmo = ppmoutpin;

  v = json["buttonpin"];
  if (!v.isNull()) {
    bp = v;
    setpins = true;
  }
  v = json["ppmoutpin"];
  if (!v.isNull()) {
    ppmo = v;
    setpins = true;
  }
  v = json["ppminpin"];
  if (!v.isNull()) {
    ppmi = v;
    setpins = true;
  }

  // Check and make sure none are the same if they aren't disabled
  if (setpins &&
      ((bp > 0 && (bp == ppmi || bp == ppmo)) || (ppmi > 0 && (ppmi == bp || ppmi == ppmo)) ||
       (ppmo > 0 && (ppmo == bp || ppmo == ppmi)))) {
    LOGE("FAULT! Setting Pins, cannot have duplicates");
  } else {
    // Disable all pins first, so no conflicts on change
    setButtonPin(-1);
    setPpmInPin(-1);
    setPpmOutPin(-1);

    // Enable them all
    setButtonPin(bp);
    setPpmInPin(ppmi);
    setPpmOutPin(ppmo);
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
