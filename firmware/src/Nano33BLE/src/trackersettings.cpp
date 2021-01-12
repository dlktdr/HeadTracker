
#include <stdio.h>
#include <string.h>

#include "serial.h"
#include "flash.h"
#include "io.h"

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

    tltch = 1;
    rllch = 2;
    panch = 3;

    servoreverse = 0x00;

    lppan = 75;
    lptiltroll = 75;

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
    ppmoutpin = -1;
    ppminpin = -1; 
    ppmoutinvert = false;
    ppmininvert = false;
    _ppmout = nullptr;
    _ppmin = nullptr;

    // Bluetooth defaults
    btmode = 0;

    // Features defaults
    rstonwave = false;

    // Setup button input & ppm output pins
    setButtonPin(DEF_BUTTON_IN);
    setPpmInPin(DEF_PPM_IN);
    setPpmOutPin(DEF_PPM_OUT);
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
    if(value > 0 && value <= DEF_PPM_CHANNELS)
        panch = (int)value;
}

int TrackerSettings::tiltCh() const
{
    return tltch;
}

void TrackerSettings::setTiltCh(int value)
{
    if(value > 0 && value <= DEF_PPM_CHANNELS)
        tltch = (int)value;
}

int TrackerSettings::rollCh() const
{
    return rllch;
}

void TrackerSettings::setRollCh(int value)
{
    if(value > 0 && value <= DEF_PPM_CHANNELS)
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
    if(value > 1 && value < 14) {       
        // If already have a PPM output, delete it.
        if(_ppmout != nullptr) {
            delete _ppmout;
            _ppmout = nullptr;
        }
                
        if(ppmoutpin > 0)
            pinMode(ppmoutpin,INPUT); // Disable old ppmoutpin
        ppmoutpin = value;
        pinMode(value,OUTPUT); // Assign new as output
        
        // Create a new object, pass the pointer back.       
        _ppmout = new PpmOut(digitalPinToPinName((uint16_t)ppmoutpin), DEF_PPM_CHANNELS);        
        
        // Set the inverted flag on it        
        _ppmout->setInverted(ppmoutinvert);
        
    // If value less than zero disable the pin
    } else if(value < 0) {        
        // If already have a PPM output, delete it.
        if(_ppmout != nullptr) {            
            delete _ppmout;
            _ppmout = nullptr;            
        }
        
        if(ppmoutpin > 0) {
            
            pinMode(ppmoutpin,INPUT); // Disable old ppmoutpin
            ppmoutpin = -1;
            
        }
    }
}

void TrackerSettings::setPpmInPin(int value)
{
    if(value > 1 && value < 14)  {

        // If already have a PPM output, delete it.
        if(_ppmin != nullptr) {
            delete _ppmin;
            _ppmin = nullptr;
        }

        if(ppminpin > 0)
            pinMode(ppminpin,INPUT); // Disable old ppmoutpin
        pinMode(value,INPUT); // Assign new
        ppminpin = value;

        // Create a new object, pass the pointer back.
        _ppmin = new PpmIn(digitalPinToPinName((uint16_t)ppminpin), DEF_PPM_CHANNELS);        

        // Set the inverted flag on it
        _ppmin->setInverted(ppmininvert);

    // Disabled pin, also set before switching pins around
    } else if(value < 0) {
        // If already have a PPM output, delete it.
        if(_ppmin != nullptr) {
            delete _ppmin;
            _ppmin = nullptr;
        }

        // Disable old ppminpin, set to input
        if(ppminpin > 0)  {
            pinMode(ppminpin,INPUT); 
            ppminpin = -1;
        }
    }
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

// Set Inverted
void TrackerSettings::setInvertedPpmOut(bool inv) 
{
    ppmoutinvert = inv;
    if(_ppmout != nullptr) {
        _ppmout->setInverted(inv);
    }
}    

// Set Inverted on PPMin
void TrackerSettings::setInvertedPpmIn(bool inv) 
{
    ppmininvert = inv;
    if(_ppmin != nullptr) {
        _ppmin->setInverted(inv);
    }
}    

int TrackerSettings::blueToothMode() const
{
    return btmode;
}

void TrackerSettings::setBlueToothMode(int mode)
{
    if(mode >0 && mode < 2)
        btmode = mode;

    // Change the Bluetooth Mode Here
    // Ideas for modes....
    //  0 = Disabled
    //  1 = Para Wireless Trainer
    //  2 = HM10 + Nano Remote
    //  3 = PC-For Updates?
}
//----------------------------------------------------------------------------------------
// Data sent the PC, for calibration and info


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

// Used for the BlueTooth Thread
void TrackerSettings::getPPMValues(uint16_t &t, uint16_t &r, uint16_t &p)
{
    t = tiltout;
    r = rollout;
    p = panout;
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

// Reset On Wave    
    v = json["rstonwave"]; if(!v.isNull()) setResetOnWave(v);

// Button and PIns
    // Disable all pins first
    setButtonPin(-1);
    setPpmInPin(-1);
    setPpmOutPin(-1);

    // Get the pins
    int bp=-1,ppmi=-1,ppmo=-1;
    v = json["buttonpin"]; if(!v.isNull()) bp=v;
    v = json["ppmoutpin"]; if(!v.isNull()) ppmo=v;
    v = json["ppminpin"]; if(!v.isNull()) ppmi=v;

    // Check and make sure none are the same if they aren't disabled
    if((bp   > 0 && (bp == ppmi || bp == ppmo)) || 
       (ppmi > 0 && (ppmi == bp || ppmi == ppmo)) ||
       (ppmo > 0 && (ppmo == bp || ppmo == ppmi))) {
        serialWriteln("HT: FAULT! Setting Pins, cannot have duplicates");
    } else {
        setButtonPin(bp);
        setPpmOutPin(ppmo);
        setPpmInPin(ppmi);
    }

    v = json["ppmininvert"]; if(!v.isNull()) setInvertedPpmIn(v);    
    v = json["ppmoutinvert"]; if(!v.isNull()) setInvertedPpmOut(v);

// Calibrarion Values
    v = json["magxoff"];
    v1 =json["magyoff"];
    v2 =json["magzoff"];

    if(!v.isNull() && !v1.isNull() && !v2.isNull())
    {   
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
        //serialWriteln("HT: Gyr offsets set");
    }

// Calibrarion Values
    v = json["accxoff"]; 
    v1 =json["accyoff"]; 
    v2 =json["acczoff"]; 

    if(!v.isNull() && !v1.isNull() && !v2.isNull())
    {   
        setAccOffset(v,v1,v2);
        //serialWriteln("HT: Acc offsets set");
    }

}

void TrackerSettings::setJSONSettings(DynamicJsonDocument &json)
{
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

    json["rllch"] = rllch;
    json["panch"] = panch;
    json["tltch"] = tltch;

    json["servoreverse"] = servoreverse;

    json["lppan"] = lppan;
    json["lptiltroll"] = lptiltroll;
    //json["gyroweightpan"] = gyroweightpan;
    //json["gyroweighttiltroll"] = gyroweighttiltroll;

    json["buttonpin"] = buttonpin;
    json["ppmoutpin"] = ppmoutpin;
    json["ppmoutinvert"] = ppmoutinvert;
    
    json["ppminpin"] = ppminpin;
    json["ppmininvert"] = ppmininvert;    

    json["btmode"] = btmode;
    json["rstonwave"] = rstonwave;

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

void TrackerSettings::saveToEEPROM()
{
    char buffer[1000];
    DynamicJsonDocument json(1000);
    setJSONSettings(json);
    int len = serializeJson(json,buffer,1000);

    if(writeFlash(buffer,len)) {
        serialWriteln("HT: Flash Write Failed");
    } else {
        serialWriteln("HT: Saved to EEPROM");
    }    
}

// Must be called on startup to create PPM object
void TrackerSettings::loadFromEEPROM()
{
    // Load Settings
    DynamicJsonDocument json(1000);
    DeserializationError de;
    de = deserializeJson(json, flashSpace);
    
    if(de != DeserializationError::Ok) 
        serialWriteln("HT: Invalid JSON Data");
    
    if(json["UUID"] == 837727) {
        serialWriteln("HT: Device has been freshly programmed");

    } else {
        serialWriteln("HT: Device contains saved code");
        loadJSONSettings(json);
    }
}

// Used to transmit raw data back to the PC
void TrackerSettings::setJSONData(DynamicJsonDocument &json)
{
    json["magx"] = roundf(magx*1000)/1000;
    json["magy"] = roundf(magy*1000)/1000;
    json["magz"] = roundf(magz*1000)/1000;

    json["gyrox"] = roundf(gyrox*1000)/1000;
    json["gyroy"] = roundf(gyroy*1000)/1000;
    json["gyroz"] = roundf(gyroz*1000)/1000;

/* Live Values for Debugging 
    json["accx"] = roundf(accx*1000)/1000;
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
*/

    json["panoff"] = roundf(panoff*1000)/1000;
    json["tiltoff"] = roundf(tiltoff*1000)/1000;
    json["rolloff"] = roundf(rolloff*1000)/1000;    
    
    json["panout"] = panout;
    json["tiltout"] = tiltout;
    json["rollout"] = rollout;

}