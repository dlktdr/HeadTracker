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

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "soc_flash.h"
#include "io.h"
#include "sense.h"
#include "base64.h"

#include "trackersettings.h"

TrackerSettings::TrackerSettings()
{
    // Tilt, Roll, Pan Defaults
    rll_min = DEF_MIN_PWM;
    rll_max = DEF_MAX_PWM;
    rll_gain =  DEF_GAIN;
    rll_cnt = PPM_CENTER;

    pan_min = DEF_MIN_PWM;
    pan_max = DEF_MAX_PWM;
    pan_gain =  DEF_GAIN;
    pan_cnt = PPM_CENTER;

    tlt_min = DEF_MIN_PWM;
    tlt_max = DEF_MAX_PWM;
    tlt_gain =  DEF_GAIN;
    tlt_cnt = PPM_CENTER;

    tltch = DEF_TILT_CH;
    rllch = DEF_ROLL_CH;
    panch = DEF_PAN_CH;

    // Servo Reversed bits
    servoreverse = 0x00;

    // Low Pass Filter
    lppan = DEF_LP_PAN;
    lptiltroll = DEF_LP_TLTRLL;

    // Sensor Offsets
    magxoff=0; magyoff=0; magzoff=0;
    accxoff=0; accyoff=0; acczoff=0;
    gyrxoff=0; gyryoff=0; gyrzoff=0;

    // Soft Iron Mag Offset
    magsioff[0] = 1; magsioff[1] = 0; magsioff[2] = 0;
    magsioff[3] = 0; magsioff[4] = 1; magsioff[5] = 0;
    magsioff[6] = 0; magsioff[7] = 0; magsioff[8] = 1;

    // Define Data Variables from X Macro
    #define DV(DT, NAME, DIV, ROUND) NAME = 0;
        DATA_VARS
    #undef DV

    // Fill all arrays to zero
    #define DA(DT, NAME, SIZE, DIV) memset(NAME, 0, sizeof(DT)*SIZE);
        DATA_ARRAYS
    #undef DA

    // Default data outputs, Pan,Tilt,Roll outputs and Inputs for Graph and Output bars
    senddatavars = 0;
    senddataarray = 0;

    // PPM Defaults
    ppmoutpin = DEF_PPM_OUT;
    ppmfrm = DEF_PPM_FRAME;
    ppmsync = DEF_PPM_SYNC;
    ppmchcnt = DEF_PPM_CHANNELS;
    ppminpin = DEF_PPM_IN;
    buttonpin = DEF_BUTTON_IN;
    ppmoutinvert = false;
    ppmininvert = false;
    rstppm = DEF_RST_PPM;

    // PWM defaults
    pwm[0] = DEF_PWM_A0_CH;
    pwm[1] = DEF_PWM_A1_CH;
    pwm[2] = DEF_PWM_A2_CH;
    pwm[3] = DEF_PWM_A3_CH;

    // Analog defaults
    an5ch = DEF_ALG_A5_CH;
    an5gain = DEF_ALG_GAIN;
    an5off = DEF_ALG_OFFSET;
    an6ch = DEF_ALG_A6_CH;
    an6gain = DEF_ALG_GAIN;
    an6off = DEF_ALG_OFFSET;
    an7ch = DEF_ALG_A7_CH;
    an7gain = DEF_ALG_GAIN;
    an7off = DEF_ALG_OFFSET;

    // AUX defaults
    aux0ch = DEF_AUX_CH0;
    aux1ch = DEF_AUX_CH1;
    aux0func = DEF_AUX_FUNC;
    aux1func = DEF_AUX_FUNC;

    // Bluetooth defaults
    btmode = 0;
    btcon = false;
    btpairedaddress[0] = 0;

    // Features defaults
    rstonwave = false;
    isCalibrated = false;
    rotx = DEF_BOARD_ROT_X;
    roty = DEF_BOARD_ROT_Y;
    rotz = DEF_BOARD_ROT_Z;

    // Setup button input & ppm output pins, bluetooth
    setButtonPin(buttonpin);
    setPpmInPin(ppminpin);
    setPpmOutPin(ppmoutpin);
    setBlueToothMode(btmode);
}

int TrackerSettings::Rll_min() const
{
    return rll_min;
}

void TrackerSettings::setRll_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    rll_min = value;
}

int TrackerSettings::Rll_max() const
{
    return rll_max;
}

void TrackerSettings::setRll_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    rll_max = value;
}

float TrackerSettings::Rll_gain() const
{
    return rll_gain;
}

void TrackerSettings::setRll_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    rll_gain = value;
}

int TrackerSettings::Rll_cnt() const
{
    return rll_cnt;
}

void TrackerSettings::setRll_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    rll_cnt = value;
}

int TrackerSettings::Pan_min() const
{
    return pan_min;
}

void TrackerSettings::setPan_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    pan_min = value;
}

int TrackerSettings::Pan_max() const
{
    return pan_max;
}

void TrackerSettings::setPan_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    pan_max = value;
}

float TrackerSettings::Pan_gain() const
{
    return pan_gain;
}

void TrackerSettings::setPan_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    pan_gain = value;
}

int TrackerSettings::Pan_cnt() const
{
    return pan_cnt;
}

void TrackerSettings::setPan_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    pan_cnt = value;
}

int TrackerSettings::Tlt_min() const
{
    return tlt_min;
}

void TrackerSettings::setTlt_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    tlt_min = value;
}

int TrackerSettings::Tlt_max() const
{
    return tlt_max;
}

void TrackerSettings::setTlt_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    tlt_max = value;
}

float TrackerSettings::Tlt_gain() const
{
    return tlt_gain;
}

void TrackerSettings::setTlt_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    tlt_gain = value;
}

int TrackerSettings::Tlt_cnt() const
{
    return tlt_cnt;
}

void TrackerSettings::setTlt_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    tlt_cnt = value;
}

int TrackerSettings::lpTiltRoll() const
{
    return lptiltroll;
}

void TrackerSettings::setLPTiltRoll(int value)
{
    lptiltroll = value;
}

int TrackerSettings::lpPan() const
{
    return lppan;
}

void TrackerSettings::setLPPan(int value)
{
    lppan = value;
}

char TrackerSettings::servoReverse() const
{
    return servoreverse;
}

void TrackerSettings::setServoreverse(char value)
{
    servoreverse = (int)value;
}

void TrackerSettings::setRollReversed(bool value)
{
    if(value)
        servoreverse |= HT_ROLL_REVERSE_BIT;
    else
        servoreverse &= (HT_ROLL_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setPanReversed(bool value)
{
    if(value)
        servoreverse |= HT_PAN_REVERSE_BIT;
    else
        servoreverse &= (HT_PAN_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setTiltReversed(bool value)
{
    if(value)
        servoreverse |= HT_TILT_REVERSE_BIT;
    else
        servoreverse &= (HT_TILT_REVERSE_BIT ^ 0xFFFF);
}

bool TrackerSettings::isRollReversed()
{
    return (servoreverse & HT_ROLL_REVERSE_BIT);
}

bool TrackerSettings::isTiltReversed()
{
    return (servoreverse & HT_TILT_REVERSE_BIT);
}

bool TrackerSettings::isPanReversed()
{
    return (servoreverse & HT_PAN_REVERSE_BIT);
}

int TrackerSettings::panCh() const
{
    return panch;
}

void TrackerSettings::setPanCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        panch = (int)value;
}

int TrackerSettings::tiltCh() const
{
    return tltch;
}

void TrackerSettings::setTiltCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        tltch = (int)value;
}

int TrackerSettings::rollCh() const
{
    return rllch;
}

void TrackerSettings::setRollCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        rllch = (int)value;
}

//----------------------------------------------------------------------------------------
// Remappable Buttons + PPM Output Pin + Bluetooth

int TrackerSettings::buttonPin() const
{
    return buttonpin;
}

void TrackerSettings::setButtonPin(int value)
{
    if(value > 1 && value < 14) {
        if(buttonpin > 0)
            pinMode((dpintoport[buttonpin] * 32) + dpintopin[buttonpin],GPIO_INPUT); // Disable old button pin
        pinMode((dpintoport[value] * 32) + dpintopin[value],INPUT_PULLUP);    // Button as Input
        buttonpin = value; // Save

    // Disable the Button Pin
    } else if (value < 0) {
        if(buttonpin > 0)
            pinMode((dpintoport[buttonpin] * 32) + dpintopin[buttonpin],GPIO_INPUT); // Disable old button pin
        buttonpin = -1;
    }
}

int TrackerSettings::ppmOutPin() const
{
    return ppmoutpin;
}

void TrackerSettings::setPpmOutPin(int value)
{
    if((value > 1 && value < 14) || value == -1)  {
       PpmOut_setPin(value);
       ppmoutpin = value;
    }
}

void TrackerSettings::setPpmInPin(int value)
{
    if((value > 1 && value < 14) || value == -1)  {
        PpmIn_setPin(value);
        ppminpin = value;
    }
}

int TrackerSettings::resetCntPPM() const
{
    return rstppm;
}

void TrackerSettings::setResetCntPPM(int value)
{
    if((value >= 1 && value <= 16) || value == -1)
        rstppm = value;
}

bool TrackerSettings::resetOnWave() const
{
    return rstonwave;
}

void TrackerSettings::setResetOnWave(bool value)
{
    rstonwave = value;
}

void TrackerSettings::setPWMCh(int pwmno, int pwmch)
{
    if(pwmno >= 0 && pwmno <= 3) {
        if(pwmch > 0 && pwmch <= 16)
            pwm[pwmno] = pwmch;
    }
}

void TrackerSettings::setAuxFunc0Ch(int channel)
{
    if((channel > 0 && channel <= 16) || channel == -1) {
        aux0ch = channel;
    }
}

void TrackerSettings::setAuxFunc1Ch(int channel)
{
    if((channel > 0 && channel <= 16) || channel == -1) {
        aux1ch = channel;
    }
}

void TrackerSettings::setAuxFunc0(int funct)
{
    if((funct >= AUX_GYRX && funct <= BT_RSSI) || funct == -1)
        aux0func = funct;
}

void TrackerSettings::setAuxFunc1(int funct)
{
    if((funct >= AUX_GYRX && funct <= BT_RSSI) || funct == -1)
        aux1func = funct;
}

void TrackerSettings::setAnalog5Ch(int channel)
{
    if((channel > 0 && channel <= 16) || channel == -1)
        an5ch = channel;
}

void TrackerSettings::setAnalog6Ch(int channel)
{
    if((channel > 0 && channel <= 16) || channel == -1)
        an6ch = channel;
}

void TrackerSettings::setAnalog7Ch(int channel)
{
    if((channel > 0 && channel <= 16) || channel == -1)
        an7ch = channel;
}

void TrackerSettings::gyroOffset(float &x, float &y, float &z) const
{
    x=gyrxoff;y=gyryoff;z=gyrzoff;
}

void TrackerSettings::setGyroOffset(float x,float y, float z)
{
    gyrxoff=x;gyryoff=y;gyrzoff=z;
}

void TrackerSettings::accOffset(float &x, float &y, float &z) const
{
    x=accxoff;y=accyoff;z=acczoff;
}

void TrackerSettings::setAccOffset(float x,float y, float z)
{
    accxoff=x;accyoff=y;acczoff=z;
}

void TrackerSettings::magOffset(float &x, float &y, float &z) const
{
    x=magxoff;y=magyoff;z=magzoff;
}

void TrackerSettings::setMagOffset(float x,float y, float z)
{
    magxoff=x;magyoff=y;magzoff=z;
}

// Set Inverted on PPMout
void TrackerSettings::setInvertedPpmOut(bool inv)
{
    ppmoutinvert = inv;
    PpmOut_setInverted(inv);
}

// Set Inverted on PPMin
void TrackerSettings::setInvertedPpmIn(bool inv)
{
    ppmininvert = inv;
    PpmIn_setInverted(inv);
}

//---------------------------------
// Bluetooth Settings

int TrackerSettings::blueToothMode() const
{
    return btmode;
}

void TrackerSettings::setBlueToothMode(int mode)
{
    if(mode < BTDISABLE || mode > BTSCANONLY)
        return;

    BTSetMode(btmodet(mode));
    btmode = mode;
}

void TrackerSettings::setBLEValues(uint16_t vals[BT_CHANNELS])
{
    memcpy(btch,vals,sizeof(uint16_t)*BT_CHANNELS);
}

void TrackerSettings::setSenseboard(bool sense)
{
    isSense = sense;
}

void TrackerSettings::setPairedBTAddress(const char *ha)
{
    // set to "" for pair to first available
    strncpy(btpairedaddress, ha, sizeof(btpairedaddress));
}

const char * TrackerSettings::pairedBTAddress()
{
    return btpairedaddress;
}

//----------------------------------------------------------------
// Orentation

void TrackerSettings::setOrientation(int rx, int ry, int rz)
{
    rotx = rx;
    roty = ry;
    rotz = rz;
    reset_fusion(); // Cause imu to reset
}

void TrackerSettings::orientRotations(float rot[3])
{
    rot[0] = rotx;
    rot[1] = roty;
    rot[2] = rotz;
}

//----------------------------------------------------------------------------------------
// Data sent the PC, for calibration and info

void TrackerSettings::setBLEAddress(const char *addr)
{
    strcpy(btaddr, addr);
}

void TrackerSettings::setDiscoveredBTHead(const char *addr)
{
    strcpy(btrmt, addr);
}

void TrackerSettings::setPPMInValues(uint16_t vals[16])
{
    for(int i=0;i<16;i++)
        ppmch[i] = vals[i];
}

void TrackerSettings::setSBUSValues(uint16_t vals[16])
{
    for(int i=0;i<16;i++)
        sbusch[i] = vals[i];
}

void TrackerSettings::setChannelOutValues(uint16_t vals[16])
{
    for(int i=0;i<16;i++)
        chout[i] = vals[i];
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
    tilt=t;
    roll=r;
    pan=p;
}

void TrackerSettings::setOffOrient(float t, float r, float p)
{
    tiltoff=t;
    rolloff=r;
    panoff=p;
}

void TrackerSettings::setPPMOut(uint16_t t, uint16_t r, uint16_t p)
{
    tiltout=t;
    rollout=r;
    panout=p;
}

void TrackerSettings::setQuaternion(float q[4])
{
    memcpy(quat, q,sizeof(float)*4);
}

//--------------------------------------------------------------------------------------
// Send and receive the data from PC
// Takes the JSON and loads the settings into the local class

void TrackerSettings::loadJSONSettings(DynamicJsonDocument &json)
{
// Channels
    JsonVariant v,v1,v2;

// Channels
    v = json["rllch"]; if(!v.isNull()) setRollCh(v);
    v = json["tltch"]; if(!v.isNull()) setTiltCh(v);
    v = json["panch"]; if(!v.isNull()) setPanCh(v);

// Servo Reversed
    v = json["servoreverse"]; if(!v.isNull()) setServoreverse(v);

// Roll
    v = json["rll_min"];  if(!v.isNull()) setRll_min(v);
    v = json["rll_max"];  if(!v.isNull()) setRll_max(v);
    v = json["rll_gain"]; if(!v.isNull()) setRll_gain(v);
    v = json["rll_cnt"];  if(!v.isNull()) setRll_cnt(v);

// Tilt
    v = json["tlt_min"];  if(!v.isNull()) setTlt_min(v);
    v = json["tlt_max"];  if(!v.isNull()) setTlt_max(v);
    v = json["tlt_gain"]; if(!v.isNull()) setTlt_gain(v);
    v = json["tlt_cnt"];  if(!v.isNull()) setTlt_cnt(v);

// Pan
    v = json["pan_min"];  if(!v.isNull()) setPan_min(v);
    v = json["pan_max"];  if(!v.isNull()) setPan_max(v);
    v = json["pan_gain"]; if(!v.isNull()) setPan_gain(v);
    v = json["pan_cnt"];  if(!v.isNull()) setPan_cnt(v);

// Misc Gains
    v = json["lppan"];              if(!v.isNull()) setLPPan(v);
    v = json["lptiltroll"];         if(!v.isNull()) setLPTiltRoll(v);

// Bluetooth
    v = json["btmode"]; if(!v.isNull()) setBlueToothMode(v);
    v = json["btpair"]; if(!v.isNull()) setPairedBTAddress(v);

// Orientation
   v = json["rotx"]; if(!v.isNull()) setOrientation(v,roty,rotz);
   v = json["roty"]; if(!v.isNull()) setOrientation(rotx,v,rotz);
   v = json["rotz"]; if(!v.isNull()) setOrientation(rotx,roty,v);

// Reset On Wave
    v = json["rstonwave"]; if(!v.isNull()) setResetOnWave(v);

// Button and Pins
    bool setpins=false;
    int bp=buttonpin;
    int ppmi=ppminpin;
    int ppmo=ppmoutpin;

    v = json["buttonpin"]; if(!v.isNull()) {bp=v; setpins=true;}
    v = json["ppmoutpin"]; if(!v.isNull()) {ppmo=v; setpins=true;}
    v = json["ppminpin"]; if(!v.isNull()) {ppmi=v; setpins=true;}

    // Check and make sure none are the same if they aren't disabled
    if(setpins && (
       (bp   > 0 && (bp == ppmi || bp == ppmo)) ||
       (ppmi > 0 && (ppmi == bp || ppmi == ppmo)) ||
       (ppmo > 0 && (ppmo == bp || ppmo == ppmi)))) {
        serialWriteln("HT: FAULT! Setting Pins, cannot have duplicates");
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

    v = json["ppmininvert"]; if(!v.isNull()) setInvertedPpmIn(v);
    v = json["ppmoutinvert"]; if(!v.isNull()) setInvertedPpmOut(v);

// Reset center on PPM Ch > 1800us
    v = json["rstppm"]; if(!v.isNull()) setResetCntPPM(v);

// PPM Data
    v = json["ppmfrm"]; if(!v.isNull()) setPPMFrame(v);
    v = json["ppmchcnt"]; if(!v.isNull()) setPpmChCount(v);
    v = json["ppmsync"]; if(!v.isNull()) setPPMSync(v);

// PWM Settings
    v = json["pwm0"]; if(!v.isNull()) setPWMCh(0,v);
    v = json["pwm1"]; if(!v.isNull()) setPWMCh(1,v);
    v = json["pwm2"]; if(!v.isNull()) setPWMCh(2,v);
    v = json["pwm3"]; if(!v.isNull()) setPWMCh(3,v);

// Analog Settings
    v = json["an5ch"]; if(!v.isNull()) setAnalog5Ch(v);
    v = json["an5off"]; if(!v.isNull()) setAnalog5Offset(v);
    v = json["an5gain"]; if(!v.isNull()) setAnalog5Gain(v);
    v = json["an6ch"]; if(!v.isNull()) setAnalog6Ch(v);
    v = json["an6off"]; if(!v.isNull()) setAnalog6Offset(v);
    v = json["an6gain"]; if(!v.isNull()) setAnalog6Gain(v);
    v = json["an7ch"]; if(!v.isNull()) setAnalog7Ch(v);
    v = json["an7off"]; if(!v.isNull()) setAnalog7Offset(v);
    v = json["an7gain"]; if(!v.isNull()) setAnalog7Gain(v);

// AUX Settings
    v = json["aux0func"]; if(!v.isNull()) setAuxFunc0(v);
    v = json["aux0ch"]; if(!v.isNull()) setAuxFunc0Ch(v);
    v = json["aux1func"]; if(!v.isNull()) setAuxFunc1(v);
    v = json["aux1ch"]; if(!v.isNull()) setAuxFunc1Ch(v);

// Calibrarion Values
    v = json["magxoff"];
    v1 =json["magyoff"];
    v2 =json["magzoff"];

    if(!v.isNull() && !v1.isNull() && !v2.isNull())
    {
        // If all zero it's not actually calibrated
        if(v != 0.0 || v1 != 0.0 || v2 != 0) {
            isCalibrated = true; // Add a notify flag calibration is complete
        }
        setMagOffset(v,v1,v2);
        //serialWriteln("HT: Mag offsets set");
    }

    // Soft Iron Offsets
    v = json["so00"]; if(!v.isNull()) magsioff[0] = v;
    v = json["so01"]; if(!v.isNull()) magsioff[1] = v;
    v = json["so02"]; if(!v.isNull()) magsioff[2] = v;
    v = json["so10"]; if(!v.isNull()) magsioff[3] = v;
    v = json["so11"]; if(!v.isNull()) magsioff[4] = v;
    v = json["so12"]; if(!v.isNull()) magsioff[5] = v;
    v = json["so20"]; if(!v.isNull()) magsioff[6] = v;
    v = json["so21"]; if(!v.isNull()) magsioff[7] = v;
    v = json["so22"]; if(!v.isNull()) magsioff[8] = v;

// Calibrarion Values
    v = json["gyrxoff"];
    v1 =json["gyryoff"];
    v2 =json["gyrzoff"];

    if(!v.isNull() && !v1.isNull() && !v2.isNull())
    {
        setGyroOffset(v,v1,v2);
    }

// Calibrarion Values
    v = json["accxoff"];
    v1 =json["accyoff"];
    v2 =json["acczoff"];

    if(!v.isNull() && !v1.isNull() && !v2.isNull())
    {
        setAccOffset(v,v1,v2);
    }
}

void TrackerSettings::setJSONSettings(DynamicJsonDocument &json)
{
// Channel Min/Max/Center/Gains
    json["rll_min"] = rll_min;
    json["rll_max"] = rll_max;
    json["rll_cnt"] = rll_cnt;
    json["rll_gain"] = rll_gain;

    json["tlt_min"] = tlt_min;
    json["tlt_max"] = tlt_max;
    json["tlt_cnt"] = tlt_cnt;
    json["tlt_gain"] = tlt_gain;

    json["pan_min"] = pan_min;
    json["pan_max"] = pan_max;
    json["pan_cnt"] = pan_cnt;
    json["pan_gain"] = pan_gain;

// Channel Numbers
    json["rllch"] = rllch;
    json["panch"] = panch;
    json["tltch"] = tltch;

// Servo Reverse Channels
    json["servoreverse"] = servoreverse;

// Gains
    json["lppan"] = lppan;
    json["lptiltroll"] = lptiltroll;

// Pins
    json["ppminpin"] = ppminpin;
    json["buttonpin"] = buttonpin;
    json["ppmoutpin"] = ppmoutpin;

// PPM Settings
    json["ppmoutinvert"] = ppmoutinvert;
    json["ppmfrm"] = ppmfrm;
    json["ppmsync"] = ppmsync;
    json["ppmchcnt"] = ppmchcnt;
    json["ppmininvert"] = ppmininvert;
    json["rstppm"] = rstppm;

// PWM Settings
    json["pwm0"] = pwm[0];
    json["pwm1"] = pwm[1];
    json["pwm2"] = pwm[2];
    json["pwm3"] = pwm[3];

// Analog Settings
    json["an5ch"] = an5ch;
    json["an5off"] = an5off;
    json["an5gain"] = an5gain;
    json["an6ch"] = an6ch;
    json["an6off"] = an6off;
    json["an6gain"] = an6gain;
    json["an7ch"] = an7ch;
    json["an7off"] = an7off;
    json["an7gain"] = an7gain;

// AUX Settings
    json["aux0func"] = aux0func;
    json["aux0ch"] = aux0ch;
    json["aux1func"] = aux1func;
    json["aux1ch"] = aux1ch;

// Bluetooth Settings
    json["btmode"] = btmode;
    json["btpair"] = btpairedaddress;

// Proximity Setting
    json["rstonwave"] = rstonwave;

// Orientation
    json["rotx"] = rotx;
    json["roty"] = roty;
    json["rotz"] = rotz;

// Calibration Values
    json["accxoff"] = accxoff;
    json["accyoff"] = accyoff;
    json["acczoff"] = acczoff;

    json["gyrxoff"] = gyrxoff;
    json["gyryoff"] = gyryoff;
    json["gyrzoff"] = gyrzoff;

    json["magxoff"] = magxoff;
    json["magyoff"] = magyoff;
    json["magzoff"] = magzoff;

// Soft Iron Offsets
    json["so00"] = magsioff[0];
    json["so01"] = magsioff[1];
    json["so02"] = magsioff[2];
    json["so10"] = magsioff[3];
    json["so11"] = magsioff[4];
    json["so12"] = magsioff[5];
    json["so20"] = magsioff[6];
    json["so21"] = magsioff[7];
    json["so22"] = magsioff[8];
}

// Saves current data to flash
void TrackerSettings::saveToEEPROM()
{
    char buffer[TX_RNGBUF_SIZE];
    DynamicJsonDocument json(JSON_BUF_SIZE);
    setJSONSettings(json);
    int len = serializeJson(json,buffer,TX_RNGBUF_SIZE);

    if(socWriteFlash(buffer,len)) {
        serialWriteln("HT: Flash Write Failed");
    } else {
        serialWriteln("HT: Saved to Flash");
    }
}

// Called on startup to read the data from Flash

void TrackerSettings::loadFromEEPROM()
{
    // Load Settings
    DynamicJsonDocument json(JSON_BUF_SIZE);
    DeserializationError de;
    de = deserializeJson(json, get_flashSpace());

    if(de != DeserializationError::Ok)
        serialWriteln("HT: Invalid JSON Data");

    if(json["UUID"] == 837727) {
        serialWriteln("HT: Device has been freshly programmed, no data found");

    } else {
        serialWriteln("HT: Loading settings from flash");
        loadJSONSettings(json);
    }
}

/* Sets if a data item should be included while in data to GUI
 */

void TrackerSettings::setDataItemSend(const char *var, bool enabled)
{
    int id=0;

    // Macro Expansion for Data Variables + Arrays
    #define DV(DT, NAME, DIV, ROUND)\
    if(strcmp(var,#NAME)==0)\
    {\
        enabled==true?senddatavars|=1<<id:senddatavars&=~(1<<id);\
        return;\
    }\
    id++;
        DATA_VARS
    #undef DV

    id=0;

    #define DA(DT, NAME, SIZE, DIV)\
    if(strcmp(var,#NAME)==0)\
    {\
        enabled==true?senddataarray|=1<<id:senddataarray&=~(1<<id);\
        return;\
    }\
    id++;
        DATA_ARRAYS
    #undef DA
}

/* Stops all Data Items from Sending
 */

void TrackerSettings::stopAllData()
{
    senddatavars = 0;
    senddataarray = 0;
}

/* Returns a list of all the Data Variables available in json
 */

void TrackerSettings::setJSONDataList(DynamicJsonDocument &json)
{
    JsonArray array = json.createNestedArray();

    // Macro Expansion for Data Variables + Arrays
    #define DV(DT, NAME, DIV, ROUND)\
        array.add(#NAME);
        DATA_VARS
    #undef DV

    #define DA(DT, NAME, SIZE, DIV)\
        array.add(#NAME);
        DATA_ARRAYS
    #undef DA
}

// Used to transmit raw data back to the GUI
void TrackerSettings::setJSONData(DynamicJsonDocument &json)
{
    static int counter=0;
    // Macro Expansion for Data Variables + Arrays

    // Sends only requested data items
    // Updates only as often as specified, 1 = every cycle
    // Three Decimals is most precision of any data item req as of now.
    // For most items ends up less bytes than base64 encoding everything
    int id=0;
    int itemcount=0;
    #define DV(DT, NAME, DIV, ROUND)\
    if(senddatavars & 1<<id && counter % DIV == 0) {\
        if(ROUND == -1)\
            json[#NAME] = NAME;\
        else\
            json[#NAME] = roundf(((float)NAME * ROUND)) / ROUND;\
        itemcount++;\
    }\
    id++;
        DATA_VARS
    #undef DV

    // Send only requested data arrays, arrays are base64 encoded
    // Variable names prepended by 6 so GUI knows to decode them
    // Suffixed with the 3 character data type
    // Length can be determined with above info

    // If DIVisor less than zero only send on change.
    // If less than -1 update on cycle count or change

    id=0;
    char b64array[500];
    bool sendit=false;

    #define DA(DT, NAME, SIZE, DIV)\
    sendit=false;\
    if(senddataarray & 1<<id) {\
        if(DIV < 0) { \
            if(memcmp(last ## NAME, NAME, sizeof(NAME)) != 0)\
                sendit = true;\
            else if(DIV != -1 && counter % abs(DIV) == 0)\
                sendit = true;\
        } else { \
            if(counter % DIV == 0)\
                sendit = true;\
        }\
        if(sendit) {\
            encode_base64((unsigned char*)NAME, sizeof(DT)*SIZE,(unsigned char*)b64array);\
            json["6" #NAME #DT] = b64array;\
            memcpy(last ## NAME, NAME, sizeof(NAME));\
        }\
    }\
    id++;
        DATA_ARRAYS
    #undef DA

    // Used for reduced data divisor
    counter++;
    if(counter > 500) {
        counter = 0;
    }
}

