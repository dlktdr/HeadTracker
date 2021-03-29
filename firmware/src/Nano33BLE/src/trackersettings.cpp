
#include <stdio.h>
#include <string.h>

#include "serial.h"
#include "flash.h"
#include "io.h"
#include "sense.h"

#include "trackersettings.h"

using namespace mbed;

TrackerSettings::TrackerSettings()
{
    // Defaults
    rll_min = DEF_MIN_PWM;
    rll_max = DEF_MAX_PWM;
    rll_gain =  DEF_GAIN;
    rll_cnt = DEF_CENTER;

    pan_min = DEF_MIN_PWM;
    pan_max = DEF_MAX_PWM;
    pan_gain =  DEF_GAIN;
    pan_cnt = DEF_CENTER;

    tlt_min = DEF_MIN_PWM;
    tlt_max = DEF_MAX_PWM;
    tlt_gain =  DEF_GAIN;
    tlt_cnt = DEF_CENTER;

    tltch = DEF_TILT_CH;
    rllch = DEF_ROLL_CH;
    panch = DEF_PAN_CH;

    servoreverse = 0x00;

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

    // Output values
    gyrox=0;gyroy=0;gyroz=0;
    accx=0;accy=0;accz=0;
    magx=0,magy=0,magz=0;
    tilt=0,roll=0,pan=0;
    panout=0,tiltout=0,rollout=0;

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

    // Bluetooth defaults
    btmode = 0;
    btcon = false;
    _btf = nullptr;

    // Features defaults
    rstonwave = false;
    isCalibrated = false;
    orient = 0;

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
            pinMode(buttonpin,INPUT); // Disable old button pin
        pinMode(value,INPUT_PULLUP);    // Button as Input
        buttonpin = value; // Save
        io_Init();

    // Disable the Button Pin
    } else if (value < 0) {
        if(buttonpin > 0)
            pinMode(buttonpin,INPUT); // Disable old button pin
        buttonpin = -1;
        io_Init();
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
    if(mode < BTDISABLE || mode > BTHM10)
        return;

    // Delete old bluetooth if changing
    if(_btf != nullptr) {
        if(mode != btmode) {
            delete _btf;
            _btf = nullptr;
        }
    }

    // Save new mode
    btmode = mode;

    // Create new bluetooth
    if(_btf == nullptr) {
        // Disabled
        if(mode == BTDISABLE) {
            sprintf(bleaddress,"00:00:00:00:00:00");

        // PARA FRSky Mode
        } else if(mode == BTPARA) {
            _btf = new BTPara();
        // BTHM10 PPM Output
        } else if(mode == BTHM10) {
            //_btf = new BTHm10();
        }
    }
}

//----------------------------------------------------------------
// Orentation

int TrackerSettings::orientation()
{
    return orient;
}

void TrackerSettings::setOrientation(int ori)
{
    if(ori >= 0 && ori <= 6) {
        orient = ori;
        reset_fusion();
    }
}

void TrackerSettings::orientRotations(float rot[3])
{
    switch(orient) {
        case 0: // Default
            rot[0] = 0; rot[1] = 0; rot[2] = 0;
            break;
        case 1: // Tilt 90
            rot[0] = 90; rot[1] = 0; rot[2] = 0;
            break;
        case 2: // Tilt -90
            rot[0] = -90; rot[1] = 0; rot[2] = 0;
            break;
        case 3: // Tilt 180
            rot[0] = 180; rot[1] = 0; rot[2] = 0;
            break;
        case 4: // Roll 90
            rot[0] = 0; rot[1] = 90; rot[2] = 0;
            break;
        case 5: // Roll -90
            rot[0] = 0; rot[1] = -90; rot[2] = 0;
            break;
        case 6: // Tilt 90, Pan 90
            rot[0] = 90; rot[1] = 0; rot[2] = 90;
            break;
        default:
            rot[0] = 0; rot[1] = 0; rot[2] = 0;
    }
}


//----------------------------------------------------------------------------------------
// Data sent the PC, for calibration and info

void TrackerSettings::setBLEAddress(const char *addr)
{
    strcpy(bleaddress,addr);
}

void TrackerSettings::setPPMInValues(uint16_t *vals, int chans)
{
    if(chans > 16)
        return;

    for(int i=0;i<chans;i++)
        ppminvals[i] = vals[i];

    ppminchans = chans;
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

// Bluetooth Mode
    v = json["btmode"]; if(!v.isNull()) setBlueToothMode(v);

// Orientation
   v = json["orient"]; if(!v.isNull()) setOrientation(v);

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

    json["servoreverse"] = servoreverse;

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

// Bluetooth Settings
    json["btmode"] = btmode;

// Proximity Setting
    json["rstonwave"] = rstonwave;

// Orientation
    json["orient"] = orient;

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
    char buffer[RX_BUF_SIZE];
    DynamicJsonDocument json(RX_BUF_SIZE);
    setJSONSettings(json);
    int len = serializeJson(json,buffer,RX_BUF_SIZE);

    if(writeFlash(buffer,len)) {
        serialWriteln("HT: Flash Write Failed");
    } else {
        serialWriteln("HT: Saved to EEPROM");
    }
}

// Called on startup to read the data from Flash

void TrackerSettings::loadFromEEPROM()
{
    // Load Settings
    DynamicJsonDocument json(RX_BUF_SIZE);
    DeserializationError de;
    de = deserializeJson(json, flashSpace);

    if(de != DeserializationError::Ok)
        serialWriteln("HT: Invalid JSON Data");

    if(json["UUID"] == 837727) {
        serialWriteln("HT: Device has been freshly programmed, no data found");

    } else {
        serialWriteln("HT: Loading settings from flash");
        loadJSONSettings(json);
    }
}

extern uint16_t zaccelout;

// Used to transmit raw data back to the GUI
void TrackerSettings::setJSONData(DynamicJsonDocument &json)
{
    json["magx"] = roundf(magx*1000)/1000;
    json["magy"] = roundf(magy*1000)/1000;
    json["magz"] = roundf(magz*1000)/1000;

    json["gyrox"] = roundf(gyrox*1000)/1000;
    json["gyroy"] = roundf(gyroy*1000)/1000;
    json["gyroz"] = roundf(gyroz*1000)/1000;

    json["panoff"] = roundf(panoff*1000)/1000;
    json["tiltoff"] = roundf(tiltoff*1000)/1000;
    json["rolloff"] = roundf(rolloff*1000)/1000;

    json["panout"] = panout;
    json["tiltout"] = tiltout;
    json["rollout"] = rollout;

    // Create string for PpmIn Chans
    char pstr[120];
    sprintf(pstr,"#CH=%d ",ppminchans);
    for(int i=0;i<ppminchans; i++) {
        sprintf(pstr,"%s %d",pstr,ppminvals[i]);
    }
    json["ppmin"] = pstr;

    // Items that don't need to be updated often
    static uint16_t slowrate=0;
    if(slowrate++ > 25) {
        json["btaddr"] = bleaddress;
        json["btcon"] = btcon;
        json["magcal"] = isCalibrated;
        slowrate = 0;
    }

    // Custom Output Option
    //json["heave"] = roundf(zaccelout * 1000)/1000;

// Live Values for Debugging.
    /*json["accx"] = roundf(accx*1000)/1000;
    json["accy"] = roundf(accy*1000)/1000;
    json["accz"] = roundf(accz*1000)/1000;

    json["offmagx"] = roundf(off_magx*1000)/1000;
    json["offmagy"] = roundf(off_magy*1000)/1000;
    json["offmagz"] = roundf(off_magz*1000)/1000;

    json["offgyrox"] = roundf(off_gyrox*1000)/1000;
    json["offgyroy"] = roundf(off_gyroy*1000)/1000;
    json["offgyroz"] = roundf(off_gyroz*1000)/1000;

    json["offaccx"] = roundf(accx*1000)/1000;
    json["offaccy"] = roundf(accy*1000)/1000;
    json["offaccz"] = roundf(accz*1000)/1000;

    json["tiltraw"] = roundf(tilt*1000)/1000;
    json["rollraw"] = roundf(roll*1000)/1000;
    json["panraw"] = roundf(pan*1000)/1000;

    json["quat0"] = roundf(quat[0]*1000)/1000;
    json["quat1"] = roundf(quat[1]*1000)/1000;
    json["quat2"] = roundf(quat[2]*1000)/1000;
    json["quat3"] = roundf(quat[3]*1000)/1000; */
}