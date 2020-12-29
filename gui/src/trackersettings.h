#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <QObject>
#include <QSettings>

// Axis Mapping
#define AXIS_X 0x00
#define AXIS_Y 0x01
#define AXIS_Z 0x02
#define AXES_MAP(XX,YY,ZZ) (ZZ<<4|YY<<2|XX)
#define X_REV 0x04
#define Y_REV 0x02
#define Z_REV 0x01

class TrackerSettings : public QObject
{    
    Q_OBJECT
public:
    // PWM Values here are divided by 2 plus 400 = actual uS output
    static const int MIN_PWM=1000; // 1000 us
    static const int MAX_PWM=2000; // 2000 us
    static const int DEF_MIN_PWM=1050;
    static const int DEF_MAX_PWM=1950;
    static const int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-250);
    static const int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+250);
    static const int MIN_GAIN= 0;
    static const int MAX_GAIN =500;
    static const int DEF_GAIN= 100;
    static const int HT_TILT_REVERSE_BIT    = 0x01;
    static const int HT_ROLL_REVERSE_BIT  =   0x02;
    static const int HT_PAN_REVERSE_BIT    =  0x04;
    static const int DEF_PPM_CHANNELS = 8;
    static const int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static const int DEF_PPM_OUT = 10; // Random choice
    static const int DEF_CENTER = 1500;

    TrackerSettings(QObject *parent=nullptr);
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

    uint panCh() const;
    void setPanCh(uint value);

    uint tiltCh() const;
    void setTiltCh(uint value);

    uint rollCh() const;
    void setRollCh(uint value);

    int count() const {return 22;}

    uint axisRemap() const {return _data["axisremap"].toUInt();}
    void setAxisRemap(uint value);

    uint axisSign() const {return _data["axissign"].toUInt();}
    void setAxisSign(uint value);

    void storeSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

    QVariantMap getAllData() {return _data;}
    void setAllData(const QVariantMap &data);

    QVariant getLiveData(const QString &name);
    void setLiveData(const QString &name, const QVariant &live);

    QVariantMap getLiveDataMap();
    void setLiveDataMap(const QVariantMap &livelist, bool clear=false);

    QString getHardware() {return _data["Hard"].toString();}
    QString getFWVersion() {return _data["Vers"].toString();}
    void setHardware(QString vers,QString hard);

signals:
    void rawGyroChanged(float x, float y, float z);
    void rawAccelChanged(float x, float y, float z);
    void rawMagChanged(float x, float y, float z);
    void rawOrientChanged(float t, float r, float p);
    void offOrientChanged(float t, float r, float p);
    void ppmOutChanged(int t, int r, int p);

private:
    QVariantMap _data; // Stored Data
    QVariantMap _live; // Live Data

/*    int rll_min,rll_max,rll_cnt,rll_gain;
    int tlt_min,tlt_max,tlt_cnt,tlt_gain;
    int pan_min,pan_max,pan_cnt,pan_gain;
    int tltch,rllch,panch;
    int servoreverse;
    int lppan,lptiltroll;
    int gyroweightpan;
    int gyroweighttiltroll;
    int buttonpin,ppmpin;*/


    /*float gyrox,gyroy,gyroz;
    float accx,accy,accz;
    float magx,magy,magz;
    float tilt,roll,pan;
    float tiltoff,rolloff,panoff;
    uint16_t panout,tiltout,rollout;*/

};

#endif // TRACKERSETTINGS_H
