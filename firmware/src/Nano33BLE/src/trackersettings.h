#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "PPMOut.h"

#define EXAMPLE_KV_VALUE_LENGTH 64
#define KV_KEY_LENGTH 32
#define err_code(res) MBED_GET_ERROR_CODE(res)

class TrackerSettings
{    
public:
    // PWM Values here are divided by 2 plus 400 = actual uS output
    static const int MIN_PWM=1000; // 1000 us
    static const int MAX_PWM=2000; // 2000 us
    static const int DEF_MIN_PWM=1050; 
    static const int DEF_MAX_PWM=1950; 
    static const int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-250);
    static const int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+250);
    static const int MIN_GAIN= 0;
    static const int MAX_GAIN= 2000;
    static const int DEF_GAIN= 200;
    static const int HT_TILT_REVERSE_BIT = 0x01;
    static const int HT_ROLL_REVERSE_BIT = 0x02;
    static const int HT_PAN_REVERSE_BIT  = 0x04;
    static const int DEF_PPM_CHANNELS = 8; 
    static const int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static const int DEF_PPM_OUT = 10; // Random choice
    static const int DEF_CENTER = 1500;
    //static const int DEF_CENTER = (MAX_PWM-MINPWM)/2+MIN_PWM;

    TrackerSettings();

    int Rll_min() const;
    void setRll_min(int value);

    int Rll_max() const;
    void setRll_max(int value);

    int Rll_gain() const;
    void setRll_gain(int value);

    int Rll_cnt() const;
    void setRll_cnt(int value);

    int Pan_min() const;
    void setPan_min(int value);

    int Pan_max() const;
    void setPan_max(int value);

    int Pan_gain() const;
    void setPan_gain(int value);

    int Pan_cnt() const;
    void setPan_cnt(int value);

    int Tlt_min() const;
    void setTlt_min(int value);

    int Tlt_max() const;
    void setTlt_max(int value);

    int Tlt_gain() const;
    void setTlt_gain(int value);

    int Tlt_cnt() const;
    void setTlt_cnt(int value);

    int lpPan() const;
    void setLPPan(int value);

    int lpTiltRoll() const;
    void setLPTiltRoll(int value);

    int gyroWeightTiltRoll() const;
    void setGyroWeightTiltRoll(int value);

    int gyroWeightPan() const;
    void setGyroWeightPan(int value);

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

    void gyroOffset(float &x, float &y, float &z) {x=gyrxoff;y=gyryoff;z=gyrzoff;}
    void setgyroOffset(float x,float y, float z) {gyrxoff=x;gyryoff=y;gyrzoff=z;}

    void accOffset(float &x, float &y, float &z) {x=accxoff;y=accyoff;z=acczoff;}
    void setAccOffset(float x,float y, float z) {accxoff=x;accyoff=y;acczoff=z;}

    void magOffset(float &x, float &y, float &z) {x=gyrxoff;y=gyryoff;z=gyrzoff;}
    void setMagOffset(float x,float y, float z) {gyrxoff=x;gyryoff=y;gyrzoff=z;} 

    int buttonPin() const;
    void setButtonPin(int value);

    int ppmPin() const;
    void setPPMPin(int value);

    // Future use where channel number adjustable
    int ppmChannels() {return DEF_PPM_CHANNELS;}

    void loadJSONSettings(DynamicJsonDocument &json);
    void setJSONSettings(DynamicJsonDocument &json);

    void saveToEEPROM();
    void loadFromEEPROM(PpmOut **ppout);

    void setInvertedPPM(bool inv);
    bool isInverted() {return ppminvert;}

// Setting of data to be returned to the PC
    void setRawGyro(float x, float y, float z);
    void setRawAccel(float x, float y, float z);
    void setRawMag(float x, float y, float z);
    void setRawOrient(float t, float r, float p);
    void setOffOrient(float t, float r, float p);
    void setPPMOut(uint16_t t, uint16_t r, uint16_t p);
    void setJSONData(DynamicJsonDocument &json);
    void getPPMValues(uint16_t &t, uint16_t &r, uint16_t &p);

    PpmOut *getPpmOut() {return _ppm;}

private:
// Saved Settings
    int rll_min,rll_max,rll_cnt,rll_gain;
    int tlt_min,tlt_max,tlt_cnt,tlt_gain;
    int pan_min,pan_max,pan_cnt,pan_gain;
    int tltch,rllch,panch;
    float magxoff, magyoff, magzoff;
    float accxoff, accyoff, acczoff;
    float gyrxoff, gyryoff, gyrzoff;
    int servoreverse;
    int lppan,lptiltroll;
    int gyroweightpan;
    int gyroweighttiltroll;
    int buttonpin,ppmpin;
    bool ppminvert;
    PpmOut *_ppm; // Local reference to PPM output Class

// Data
    float gyrox,gyroy,gyroz;
    float accx,accy,accz;
    float magx,magy,magz;
    float tilt,roll,pan;
    float tiltoff,rolloff,panoff;
    uint16_t panout,tiltout,rollout;
};
    
#endif