#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#define MIN_PWM 575
#define MAX_PWM 1725
#define MIN_CNT (((MAX_PWM-MIN_PWM)/2)+MIN_PWM-250)
#define MAX_CNT (((MAX_PWM-MIN_PWM)/2)+MIN_PWM+250)
#define MIN_GAIN 0
#define MAX_GAIN 500

#define HT_TILT_REVERSE_BIT     0x01
#define HT_ROLL_REVERSE_BIT     0x02
#define HT_PAN_REVERSE_BIT      0x04

class TrackerSettings
{    
public:
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
    bool rollReversed() {return (!(servoreverse|HT_ROLL_REVERSE_BIT));}
    bool tiltReversed() {return (!(servoreverse|HT_TILT_REVERSE_BIT));}
    bool panReversed() {return (!(servoreverse|HT_PAN_REVERSE_BIT));}

    char panCh() const;
    void setPanCh(char value);

    char tiltCh() const;
    void setTiltCh(char value);

    char rollCh() const;
    void setRollCh(char value);

    int count() const {return 20;}

private:
    int rll_min,rll_max,rll_gain,rll_cnt;
    int pan_min,pan_max,pan_gain,pan_cnt;
    int tlt_min,tlt_max,tlt_gain,tlt_cnt;

    int lptiltroll;         // Firmware: tiltRollBeta
    int lppan;              // Firmware: panBeta
    int gyroweighttiltroll; // Firmware: gyroWeightTiltRoll
    int gyroweightpan;      // Firmware: GyroWeightPan

    char servoreverse;      // Firmware: servoReverseMask

    char panch;             // Firmware: htChannels[0]
    char tiltch;            // Firmware: htChannels[1]
    char rollch;            // Firmware: htChannels[2]
};

#endif // TRACKERSETTINGS_H
