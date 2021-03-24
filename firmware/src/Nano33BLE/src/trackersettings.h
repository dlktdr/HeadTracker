#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"
#include "config.h"
#include "serial.h"
#include "btpara.h"

// Global Config Values

class TrackerSettings
{
public:
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
    static constexpr uint16_t PPM_MIN_FRAME = 12500;
    static constexpr uint16_t PPM_MIN_FRAMESYNC = 4000; // Not adjustable
    static constexpr int DEF_PPM_SYNC=300;
    static constexpr int PPM_MAX_SYNC=800;
    static constexpr int PPM_MIN_SYNC=100;
    static constexpr int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static constexpr int DEF_PPM_OUT = 10; // Random choice
    static constexpr int DEF_PPM_IN = -1;
    static constexpr int DEF_CENTER = 1500;
    static constexpr float MIN_GAIN= 0.0;
    static constexpr float MAX_GAIN= 35.0;
    static constexpr float DEF_GAIN= 5.0;
    static constexpr int DEF_BT_MODE= BTDISABLE; // Bluetooth Disabled
    static constexpr int DEF_RST_PPM = -1;
    static constexpr int DEF_TILT_CH = 6;
    static constexpr int DEF_ROLL_CH = 7;
    static constexpr int DEF_PAN_CH = 8;
    static constexpr int DEF_LP_PAN = 75;
    static constexpr int DEF_LP_TLTRLL = 75;

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

    int ppmOutPin() const;
    void setPpmOutPin(int value);

    bool invertedPpmOut() {return ppmoutinvert;}
    void setInvertedPpmOut(bool inv);

    uint16_t ppmFrame() {return ppmfrm;}
    void setPPMFrame(uint16_t v) { if(v >= PPM_MIN_FRAME  && v <= PPM_MAX_FRAME) ppmfrm = v;}

    uint16_t ppmSync() {return ppmsync;}
    void setPPMSync(uint16_t v) { if(v >= PPM_MIN_SYNC && v <= PPM_MAX_SYNC) ppmsync = v;}

    int ppmChCount() {return ppmchcnt;}
    void setPpmChCount(int v) { if(v > 3 && v < 17) ppmchcnt = v;}

    int ppmInPin() const;
    void setPpmInPin(int value);

    bool invertedPppmIn() {return ppmininvert;}
    void setInvertedPpmIn(bool inv);

    int buttonPin() const;
    void setButtonPin(int value);

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

    int orientation();
    void setOrientation(int ori);
    void orientRotations(float rot[3]);

    void magSiOffset(float v[]) {memcpy(v,magsioff,9*sizeof(float));}
    void setMagSiOffset(float v[]) {memcpy(magsioff,v,9*sizeof(float));}

    void loadJSONSettings(DynamicJsonDocument &json);
    void setJSONSettings(DynamicJsonDocument &json);

    void saveToEEPROM();
    void loadFromEEPROM();

// Setting of data to be returned to the PC
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
    void setPPMInValues(uint16_t *vals, int chans);
    void setQuaternion(float q[4]);

    BTFunction *getBTFunc() {return _btf;}

private:
    // Saved Settings
    int rll_min,rll_max,rll_cnt;
    int tlt_min,tlt_max,tlt_cnt;
    int pan_min,pan_max,pan_cnt;
    float rll_gain,tlt_gain,pan_gain;
    int tltch,rllch,panch;

    // Calibration
    float magxoff, magyoff, magzoff;
    float magsioff[9];
    float accxoff, accyoff, acczoff;
    float gyrxoff, gyryoff, gyrzoff;

    int servoreverse;
    int lppan,lptiltroll;
    int buttonpin,ppmoutpin,ppminpin;
    bool ppmoutinvert;
    bool ppmininvert;
    BTFunction *_btf; // Blue tooth Function
    int btmode;
    bool btcon;
    bool rstonwave;
    bool freshProgram;
    int orient;
    int rstppm;
    uint16_t ppmfrm;
    uint16_t ppmsync;
    uint16_t ppmchcnt;

    // Data
    float gyrox,gyroy,gyroz;
    float accx,accy,accz;
    float magx,magy,magz;
    float off_gyrox,off_gyroy,off_gyroz;
    float off_accx,off_accy,off_accz;
    float off_magx,off_magy,off_magz;
    float tilt,roll,pan;
    float tiltoff,rolloff,panoff;
    float quat[4];
    uint16_t panout,tiltout,rollout;
    uint16_t ppminvals[16];
    int ppminchans;
    char bleaddress[20];
    bool isCalibrated;
};

#endif