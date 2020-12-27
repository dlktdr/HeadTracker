
#include "trackersettings.h"

TrackerSettings::TrackerSettings()
{
    // Defaults
    rll_min = MIN_PWM;
    rll_max = MAX_PWM;
    rll_gain =  DEF_GAIN;
    rll_cnt = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    pan_min = MIN_PWM;
    pan_max = MAX_PWM;
    pan_gain =  DEF_GAIN;
    pan_cnt = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    tlt_min = MIN_PWM;
    tlt_max = MAX_PWM;
    tlt_gain =  DEF_GAIN;
    tlt_cnt = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    tltch = 6;
    rllch = 7;
    panch = 8;

    servoreverse = 0x00;

    lppan = 75;
    lptiltroll = 75;

    gyroweightpan = 30;
    gyroweighttiltroll = 40;

    // Output values
    gyrox=0;gyroy=0;gyroz=0;
    accx=0;accy=0;accz=0;
    magx=0,magy=0,magz=0;
    tilt=0,roll=0,pan=0;
    panout=0,tiltout=0,rollout=0;

    // Setup button input & ppm output pins
    ppmpin = DEF_PPM_OUT; // Set initial val    
    setButtonPin(DEF_BUTTON_IN);
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

int TrackerSettings::Rll_gain() const
{
    return rll_gain;
}

void TrackerSettings::setRll_gain(int value)
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

int TrackerSettings::Pan_gain() const
{
    return pan_gain;
}

void TrackerSettings::setPan_gain(int value)
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

int TrackerSettings::Tlt_gain() const
{
    return tlt_gain;
}

void TrackerSettings::setTlt_gain(int value)
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

int TrackerSettings::gyroWeightTiltRoll() const
{
    return gyroweighttiltroll;
}

void TrackerSettings::setGyroWeightTiltRoll(int value)
{
    gyroweighttiltroll = value;
}

int TrackerSettings::gyroWeightPan() const
{
    return gyroweightpan;
}

void TrackerSettings::setGyroWeightPan(int value)
{
    gyroweightpan = value;
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
// Remappable Buttons + PPM Output Pin

int TrackerSettings::buttonPin() const
{
    return buttonpin;
}

void TrackerSettings::setButtonPin(int value)
{    
    if(value > 1 && value < 14 && value != ppmpin) {
        pinMode(value,INPUT);    // Button as Input
        digitalWrite(value,HIGH); // Pull up Nn
        buttonpin = value; // Save
    }
}

int TrackerSettings::ppmPin() const
{
    return ppmpin;
}

void TrackerSettings::setPPMPin(int value, PpmOut **ppout)
{
    if(value > 1 && value < 14 && value != buttonpin) {
        pinMode(ppmpin,INPUT); // Disable old ppmpin
        digitalWrite(ppmpin,LOW);
        pinMode(value,OUTPUT);
        ppmpin = value;

        // If already have a PPM output, delete it.
        if(*ppout != nullptr)
            delete *ppout;

        // Create a new object, pass the pointer back.
        *ppout = new PpmOut(digitalPinToPinName(ppmpin), DEF_PPM_CHANNELS);
    }
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

//--------------------------------------------------------------------------------------
// Send and receive the data from PC

void TrackerSettings::loadJSONSettings(DynamicJsonDocument &json)
{
// Channels
    JsonVariant v;

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
    v = json["gyroweightpan"];      if(!v.isNull()) setGyroWeightPan(v);
    v = json["gyroweighttiltroll"]; if(!v.isNull()) setGyroWeightTiltRoll(v);

// Button Input + PPM output pins
    v = json["buttonpin"]; if(!v.isNull()) setButtonPin(v);
    v = json["ppmpin"]; if(!v.isNull()) setButtonPin(v);
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
    json["gyroweightpan"] = gyroweightpan;
    json["gyroweighttiltroll"] = gyroweighttiltroll;

    json["buttonpin"] = buttonpin;
    json["ppmpin"] = ppmpin;
}


// TO DO SAVE TO FLASH / LOAD FROM FLASH

void TrackerSettings::saveToEEPROM()
{

}

void TrackerSettings::loadFromEEPROM(PpmOut **ppout)
{
    // Load Settings

    // Set PPM Pin, Set Button Pin, Pointer to Pointer passed to create new PPM object

    // Default until actual flash read
    setPPMPin(DEF_PPM_OUT, ppout);
}

// Used to transmit raw data back to the PC
void TrackerSettings::setJSONData(DynamicJsonDocument &json)
{
    json["magx"] = magx;
    json["magy"] = magy;
    json["magz"] = magz;

    json["gyrox"] = gyrox;
    json["gyroy"] = gyroy;
    json["gyroz"] = gyroz;

    json["accx"] = accx;
    json["accy"] = accy;
    json["accz"] = accz;

    json["tiltraw"] = tilt;
    json["rollraw"] = roll;
    json["panraw"] = pan;
    
    json["panout"] = panout;
    json["tiltout"] = tiltout;
    json["rollout"] = rollout;

    json["panoff"] = panoff;
    json["tiltoff"] = tiltoff;
    json["rolloff"] = rolloff;

}