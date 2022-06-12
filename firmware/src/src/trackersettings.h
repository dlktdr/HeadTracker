#pragma once

#include "arduinojsonwrp.h"
#include <zephyr.h>
#include "PPMOut.h"
#include "PPMIn.h"
#include "btparahead.h"
#include "btpararmt.h"
#include "serial.h"

// Variables to be sent back to GUI if enabled
// Datatype, Name, UpdateDivisor, RoundTo
#define DATA_VARS\
    DV(float,magx,       1,1000)\
    DV(float,magy,       1,1000)\
    DV(float,magz,       1,1000)\
    DV(float,gyrox,      1,1000)\
    DV(float,gyroy,      1,1000)\
    DV(float,gyroz,      1,1000)\
    DV(float,accx,       1,1000)\
    DV(float,accy,       1,1000)\
    DV(float,accz,       1,1000)\
    DV(float,off_magx,   5,1000)\
    DV(float,off_magy,   5,1000)\
    DV(float,off_magz,   5,1000)\
    DV(float,off_gyrox,  5,1000)\
    DV(float,off_gyroy,  5,1000)\
    DV(float,off_gyroz,  5,1000)\
    DV(float,off_accx,   5,1000)\
    DV(float,off_accy,   5,1000)\
    DV(float,off_accz,   5,1000)\
    DV(float,tilt,       5,1000)\
    DV(float,roll,       5,1000)\
    DV(float,pan,        5,1000)\
    DV(float,tiltoff,    1,1000)\
    DV(float,rolloff,    1,1000)\
    DV(float,panoff,     1,1000)\
    DV(uint16_t,tiltout, 1,-1)\
    DV(uint16_t,rollout, 1,-1)\
    DV(uint16_t,panout,  1,-1)\
    DV(bool,isCalibrated,2,-1)\
    DV(bool,gyroCal,     2,-1)\
    DV(bool,btcon,       10,-1)\
    DV(bool,isSense,     10,-1)\
    DV(bool,trpenabled,  10,-1)\
    DV(uint8_t, cpuuse,  1,-1)

// To shorten names, as these are sent to the GUI for decoding
#define u8  uint8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define flt float
#define chr char

// Arrays to be sent back to GUI if enabled, Base64 Encoded
// Datatype, Name, Size, UpdateDivisor
#define DATA_ARRAYS\
    DA(u16, chout, 16, 1)\
    DA(u16, btch, BT_CHANNELS, 1)\
    DA(u16, ppmch, 16, 1)\
    DA(u16, sbusch, 16, 1)\
    DA(flt, quat,4, 1)\
    DA(chr, btaddr,18, 20)\
    DA(chr, btrmt,18, -100)

// Global Config Values
class TrackerSettings
{
public:
    enum {AUX_DISABLE=-1,
        AUX_GYRX, // 0
        AUX_GYRY, // 1
        AUX_GYRZ, // 2
        AUX_ACCELX, // 3
        AUX_ACCELY, // 4
        AUX_ACCELZ, // 5
        AUX_ACCELZO, // 6
        BT_RSSI}; // 7

    static constexpr int MIN_PWM=988;
    static constexpr int MAX_PWM=2012;
    static constexpr int DEF_MIN_PWM=1050;
    static constexpr int DEF_MAX_PWM=1950;
    static constexpr int MINMAX_RNG=242;
    static constexpr int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-MINMAX_RNG);
    static constexpr int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+MINMAX_RNG);
    static constexpr int HT_TILT_REVERSE_BIT  = 0x01;
    static constexpr int HT_ROLL_REVERSE_BIT  = 0x02;
    static constexpr int HT_PAN_REVERSE_BIT   = 0x04;
    static constexpr int DEF_PPM_CHANNELS = 8;
    static constexpr uint16_t DEF_PPM_FRAME = 22500;
    static constexpr uint16_t PPM_MAX_FRAME = 40000;
    static constexpr uint16_t PPM_MIN_FRAME = 6666;
    static constexpr uint16_t PPM_MIN_FRAMESYNC = 4000; // Not adjustable
    static constexpr int DEF_PPM_SYNC=350;
    static constexpr int PPM_MAX_SYNC=800;
    static constexpr int PPM_MIN_SYNC=100;
    static constexpr int DEF_BOARD_ROT_X=0;
    static constexpr int DEF_BOARD_ROT_Y=0;
    static constexpr int DEF_BOARD_ROT_Z=0;
    static constexpr int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static constexpr bool DEF_BUTTON_LONG_PRESS = false;
    static constexpr bool DEF_RESET_ON_TILT = false;
    static constexpr float RESET_ON_TILT_TIME = 1.5; // Seconds to complete a head tilt
    static constexpr float RESET_ON_TILT_AFTER = 1.0; // How long after the tilt to reset
    static constexpr float RECENTER_PULSE_DURATION = 0.5; // (sec) pulse width of recenter signal to tx
    static constexpr int DEF_PPM_OUT = 10; // Random choice
    static constexpr int DEF_PPM_IN = -1;
    static constexpr int PPM_CENTER = 1500;
    static constexpr int SBUS_CENTER = 992;
    static constexpr float SBUS_SCALE = 1.6f;
    static constexpr float MIN_GAIN= 0.0;
    static constexpr float MAX_GAIN= 35.0;
    static constexpr float DEF_GAIN= 5.0;
    static constexpr int DEF_BT_MODE= BTDISABLE; // Bluetooth Disabled
    static constexpr int DEF_RST_PPM = -1;
    static constexpr int DEF_TILT_CH = -1;
    static constexpr int DEF_ROLL_CH = -1;
    static constexpr int DEF_PAN_CH = -1;
    static constexpr int DEF_ALERT_CH = -1;
    static constexpr int DEF_LP_PAN = 90;
    static constexpr int DEF_LP_TLTRLL = 90;
    static constexpr int DEF_PWM_A0_CH = -1;
    static constexpr int DEF_PWM_A1_CH = -1;
    static constexpr int DEF_PWM_A2_CH = -1;
    static constexpr int DEF_PWM_A3_CH = -1;
    static constexpr bool DEF_SBUS_IN_INV = true;
    static constexpr bool DEF_SBUS_OUT_INV = true;
    static constexpr int DEF_SBUS_RATE = 60;
    static constexpr float SBUS_ACTIVE_TIME = 0.1; // 10Hz
    static constexpr int DEF_ALG_A4_CH = -1;
    static constexpr int DEF_ALG_A5_CH = -1;
    static constexpr int DEF_ALG_A6_CH = -1;
    static constexpr int DEF_ALG_A7_CH = -1;
    static constexpr float DEF_ALG_GAIN = 310.0f;
    static constexpr int DEF_ALG_OFFSET = 0;
    static constexpr int DEF_AUX_CH0 = -1;
    static constexpr int DEF_AUX_CH1 = -1;
    static constexpr int DEF_AUX_CH2 = -1;
    static constexpr int DEF_AUX_FUNC = 0;
    static constexpr int MAX_DATA_VARS = 40;
    static constexpr int DEF_SERIAL_MODE = 0;

    TrackerSettings();

    int Rll_min() const;
    void setRll_min(int value);

    int Rll_max() const;
    void setRll_max(int value);

    float Rll_gain() const;
    void setRll_gain(float value);

    int Rll_cnt() const;
    void setRll_cnt(int value);

    int Pan_min() const;
    void setPan_min(int value);

    int Pan_max() const;
    void setPan_max(int value);

    float Pan_gain() const;
    void setPan_gain(float value);

    int Pan_cnt() const;
    void setPan_cnt(int value);

    int Tlt_min() const;
    void setTlt_min(int value);

    int Tlt_max() const;
    void setTlt_max(int value);

    float Tlt_gain() const;
    void setTlt_gain(float value);

    int Tlt_cnt() const;
    void setTlt_cnt(int value);

    int lpPan() const;
    void setLPPan(int value);

    int lpTiltRoll() const;
    void setLPTiltRoll(int value);

    char servoReverse() const;
    void setServoreverse(char value);
    void setRollReversed(bool value);
    void setPanReversed(bool Value);
    void setTiltReversed(bool Value);
    bool isRollReversed();
    bool isTiltReversed();
    bool isPanReversed();

    int panCh() const;
    void setPanCh(int value);

    int tiltCh() const;
    void setTiltCh(int value);

    int rollCh() const;
    void setRollCh(int value);

    int alertCh() const;
    void setAlertCh(int value);

    int ppmOutPin() const;
    void setPpmOutPin(int value);

    bool invertedPpmOut() {return ppmoutinvert;}
    void setInvertedPpmOut(bool inv);

    uint16_t ppmFrame() {return ppmfrm;}
    void setPPMFrame(uint16_t v) { if(v >= PPM_MIN_FRAME  && v <= PPM_MAX_FRAME) ppmfrm = v;}

    uint16_t ppmSync() {return ppmsync;}
    void setPPMSync(uint16_t v) { if(v >= PPM_MIN_SYNC && v <= PPM_MAX_SYNC) ppmsync = v;}

    int ppmChCount() {return ppmchcnt;}
    void setPpmChCount(int v) { if(v > 0 && v < 17) ppmchcnt = v;}

    int ppmInPin() const;
    void setPpmInPin(int value);

    bool invertedPppmIn() {return ppmininvert;}
    void setInvertedPpmIn(bool inv);

    int buttonPin() const;
    void setButtonPin(int value);

    void setButtonPressMode(bool lngpresmd) {butlngps = lngpresmd;} // True = Enable/Disable output on long press
    bool buttonPressMode() {return butlngps;}
    void setTRPEnabled(bool v) {trpenabled = v;}

    void setResetOnTilt(bool r) {rstontlt = r;}
    bool resetOnTiltMode() {return rstontlt;}

    int resetCntPPM() const;
    void setResetCntPPM(int value);

    bool resetOnWave() const;
    void setResetOnWave(bool value);

    void gyroOffset(float &x, float &y, float &z) const;
    void setGyroOffset(float x,float y, float z);

    void accOffset(float &x, float &y, float &z) const;
    void setAccOffset(float x,float y, float z);

    void magOffset(float &x, float &y, float &z) const;
    void setMagOffset(float x,float y, float z);

    int blueToothMode() const;
    void setBlueToothMode(int mode);
    bool isBlueToothConnected() {return btcon;}
    void setBlueToothConnected(bool con) {btcon = con;}

    void setPairedBTAddress(const char *ha);
    const char* pairedBTAddress();

    void setOrientation(int rx, int ry, int rz);
    void orientRotations(float rot[3]);

    void magSiOffset(float v[]) {memcpy(v,magsioff,9*sizeof(float));}
    void setMagSiOffset(float v[]) {memcpy(magsioff,v,9*sizeof(float));}

// PWM Channels
    void setPWMCh(int pwmno, int pwmch);
    int PWMCh(int pwmno) { return pwm[pwmno];}

// SBUS
    void setInvertedSBUSIn(bool v) {sbininv = v;}
    bool invertedSBUSIn() {return sbininv;}
    void setInvertedSBUSOut(bool v) {sboutinv = v;}
    bool invertedSBUSOut() {return sboutinv;}
    void setSBUSRate(uint8_t rate) { if(rate>=30 && rate<=150) sbrate = rate; }
    uint8_t SBUSRate() { return sbrate ;}

// Analogs
    void setAnalog4Ch(int channel);
    void setAnalog4Gain(float gain) {an4gain=gain;}
    void setAnalog4Offset(int offset) {an4off = offset;}
    void setAnalog5Ch(int channel);
    void setAnalog5Gain(float gain) {an5gain=gain;}
    void setAnalog5Offset(int offset) {an5off = offset;}
    void setAnalog6Ch(int channel);
    void setAnalog6Gain(float gain) {an6gain=gain;}
    void setAnalog6Offset(int offset) {an6off = offset;}
    void setAnalog7Ch(int channel);
    void setAnalog7Gain(float gain) { an7gain = gain;}
    void setAnalog7Offset(int offset) {an7off = offset;}
    int analog4Ch() {return an4ch;}
    float analog4Gain() {return an4gain;}
    int analog4Offset() {return an4off;}
    int analog5Ch() {return an5ch;}
    float analog5Gain() {return an5gain;}
    int analog5Offset() {return an5off;}
    int analog6Ch() {return an6ch;}
    float analog6Gain() {return an6gain;}
    int analog6Offset() {return an6off;}
    int analog7Ch() {return an7ch;}
    float analog7Gain() {return an7gain;}
    int analog7Offset() {return an7off;}

// Aux Func
    void setAuxFunc0Ch(int channel);
    void setAuxFunc1Ch(int channel);
    void setAuxFunc2Ch(int channel);
    void setAuxFunc0(int funct);
    void setAuxFunc1(int funct);
    void setAuxFunc2(int funct);
    int auxFunc0Ch() {return aux0ch;}
    int auxFunc1Ch() {return aux1ch;}
    int auxFunc2Ch() {return aux2ch;}
    int auxFunc0() {return aux0func;}
    int auxFunc1() {return aux1func;}
    int auxFunc2() {return aux2func;}

    void loadJSONSettings(DynamicJsonDocument &json);
    void setJSONSettings(DynamicJsonDocument &json);

    void saveToEEPROM();
    void loadFromEEPROM();

// Setting of data to be returned to the GUI
    void setRawGyro(float x, float y, float z);
    void setRawAccel(float x, float y, float z);
    void setRawMag(float x, float y, float z);
    void setRawOrient(float t, float r, float p);
    void setOffGyro(float x, float y, float z);
    void setOffAccel(float x, float y, float z);
    void setOffMag(float x, float y, float z);
    void setOffOrient(float t, float r, float p);
    void setPPMOut(uint16_t t, uint16_t r, uint16_t p);
    void setJSONData(DynamicJsonDocument &json);
    void setBLEAddress(const char *addr);
    void setDiscoveredBTHead(const char* addr);
    void setBLEValues(uint16_t vals[BT_CHANNELS]);
    void setSenseboard(bool sense);
    void setSBUSValues(uint16_t vals[16]);
    void setPPMInValues(uint16_t vals[16]);
    void setChannelOutValues(uint16_t vals[16]);
    void setQuaternion(float q[4]);
    void setDataItemSend(const char *var, bool enabled);
    void setGyroCalibrated(bool gc) {gyroCal = gc;}
    void stopAllData();
    void setJSONDataList(DynamicJsonDocument &json);

private:
    // Saved Settings
    int rll_min,rll_max,rll_cnt;
    int tlt_min,tlt_max,tlt_cnt;
    int pan_min,pan_max,pan_cnt;
    float rll_gain,tlt_gain,pan_gain;
    int tltch,rllch,panch,alertch;

    // Calibration
    float magxoff, magyoff, magzoff;
    float magsioff[9];
    float accxoff, accyoff, acczoff;
    float gyrxoff, gyryoff, gyrzoff;

    // Board Rotation
    int rotx,roty,rotz;

    int servoreverse;
    int lppan,lptiltroll;
    int buttonpin,ppmoutpin,ppminpin;
    bool butlngps;
    bool rstontlt;
    bool ppmoutinvert;
    bool ppmininvert;
    int btmode;
    int sermode;

    bool rstonwave;
    bool freshProgram;
    int rstppm;
    uint16_t ppmfrm;   // PPM Frame Len
    uint16_t ppmsync;  // Sync Setting
    uint16_t ppmchcnt; // Channel Count

    int pwm[4]; // PWM Output Pins
    int an4ch,an5ch,an6ch,an7ch; // Analog Channels
    float an4gain,an5gain,an6gain,an7gain; // Analog Gains
    float an4off, an5off, an6off, an7off; // Analog Offsets
    int aux0ch,aux1ch,aux2ch; // Auxiliary Function Channels
    int aux0func,aux1func,aux2func; // Auxiliary Functions

    bool sboutinv;
    bool sbininv;
    uint8_t sbrate;

    // Bit map of data to send to GUI, max 64 items
    uint64_t senddatavars;
    uint32_t senddataarray;

    // BT Address for remote mode to pair with
    char btpairedaddress[17];

    // Define Data Variables from X Macro
    #define DV(DT, NAME, DIV, ROUND) DT NAME;
        DATA_VARS
    #undef DV

    // Define Data Arrays from X Macro
    #define DA(DT, NAME, SIZE, DIV) DT NAME[SIZE];
        DATA_ARRAYS
    #undef DA

    // Define Last Arrays from X Macro
    #define DA(DT, NAME, SIZE, DIV) DT last ## NAME [SIZE];
        DATA_ARRAYS
    #undef DA
};

extern TrackerSettings trkset;