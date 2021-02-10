#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"
#include "config.h"
#include "serial.h"
#include "btpara.h"

//#define EXAMPLE_KV_VALUE_LENGTH 64
//#define KV_KEY_LENGTH 32
//#define err_code(res) MBED_GET_ERROR_CODE(res)


class TrackerSettings
{    
public:    
    static constexpr int MIN_PWM=1000;
    static constexpr int MAX_PWM=2000;
    static constexpr int DEF_MIN_PWM=1050;
    static constexpr int DEF_MAX_PWM=1950;
    static constexpr int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-250);
    static constexpr int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+250);
    static constexpr int HT_TILT_REVERSE_BIT  = 0x01;
    static constexpr int HT_ROLL_REVERSE_BIT  = 0x02;
    static constexpr int HT_PAN_REVERSE_BIT   = 0x04;
    static constexpr int DEF_PPM_CHANNELS = MAX_PPM_CHANNELS;
    static constexpr int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static constexpr int DEF_PPM_OUT = D10;
    static constexpr int DEF_PPM_IN = -1;
    static constexpr int DEF_CENTER = 1500;
    static constexpr float MIN_GAIN= 0;
    static constexpr float MAX_GAIN= 50.0;
    static constexpr float DEF_GAIN= 10.0;
    static constexpr int DEF_BT_MODE= BTDISABLE; // Bluetooth Disabled
    static constexpr int DEF_RST_PPM = -1;

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

    int orientation();
    void setOrientation(int ori);
    void orientRotations(float rot[3]);

    void magSiOffset(float v[]) {memcpy(v,magsioff,9*sizeof(float));}
    void setMagSiOffset(float v[]) {memcpy(magsioff,v,9*sizeof(float));}
    
    // Future use where channel number adjustable
    int ppmChannels() {return ppmchannels;}
    void setPPMChannels(int num) {}

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
   
    BTFunction *getBTFunc() {return _btf;}

private:
    // Saved Settings
    int rll_min,rll_max,rll_cnt;
    int tlt_min,tlt_max,tlt_cnt;
    int pan_min,pan_max,pan_cnt;
    float rll_gain,tlt_gain,pan_gain;
    int tltch,rllch,panch;
    int ppmchannels;
    
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
    bool rstonwave;
    bool freshProgram;    
    int orient;
    int rstppm;

    // Data
    float gyrox,gyroy,gyroz;
    float accx,accy,accz;
    float magx,magy,magz;
    float off_gyrox,off_gyroy,off_gyroz;
    float off_accx,off_accy,off_accz;
    float off_magx,off_magy,off_magz;
    float tilt,roll,pan;
    float tiltoff,rolloff,panoff;
    uint16_t panout,tiltout,rollout;
    uint16_t ppminvals[16];
    int ppmchans;
    char bleaddress[20];
    bool isCalibrated;
};
    
#endif