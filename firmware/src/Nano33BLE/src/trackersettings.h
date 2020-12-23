#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <Arduino.h>
#include "ArduinoJson.h"

class TrackerSettings
{    
public:
    // PWM Values here are divided by 2 plus 400 = actual uS output
    static const int MIN_PWM=1001; // 1000 % 2 + 400 = 900 uS
    static const int MAX_PWM= 3400; // 3400 /2 + 400 = 2100 uS
    static const int DEF_MIN_PWM=1200 ;// 1200 = 1000uS
    static const int DEF_MAX_PWM=3200; // 1200 = 1000uS
    static const int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-550);
    static const int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+550);
    static const int MIN_GAIN= 0;
    static const int MAX_GAIN =500;
    static const int HT_TILT_REVERSE_BIT    = 0x01;
    static const int HT_ROLL_REVERSE_BIT  =   0x02;
    static const int HT_PAN_REVERSE_BIT    =  0x04;
    static const int PPM_CHANNELS = 8;

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

    void setFromJSON(DynamicJsonDocument &json);
    void setToJSON(DynamicJsonDocument &json);

    void saveToEEPROM();
    void loadFromEEPROM();

private:
    int rll_min,rll_max,rll_cnt,rll_gain;
    int tlt_min,tlt_max,tlt_cnt,tlt_gain;
    int pan_min,pan_max,pan_cnt,pan_gain;
    int tltch,rllch,panch;
    int servoreverse;
    int lppan,lptiltroll;
    int gyroweightpan;
    int gyroweighttiltroll;
};
    
#endif