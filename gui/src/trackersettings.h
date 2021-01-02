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
    static constexpr int MIN_PWM=1000; // 1000 us
    static constexpr int MAX_PWM=2000; // 2000 us
    static constexpr int DEF_MIN_PWM=1050;
    static constexpr int DEF_MAX_PWM=1950;
    static constexpr int MIN_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM-250);
    static constexpr int MAX_CNT=(((MAX_PWM-MIN_PWM)/2)+MIN_PWM+250);
    static constexpr int HT_TILT_REVERSE_BIT  = 0x01;
    static constexpr int HT_ROLL_REVERSE_BIT  = 0x02;
    static constexpr int HT_PAN_REVERSE_BIT   = 0x04;
    static constexpr int DEF_PPM_CHANNELS = 8;
    static constexpr int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static constexpr int DEF_PPM_OUT = 10; // Random choice
    static constexpr int DEF_PPM_IN = 9; // Random choice
    static constexpr int DEF_CENTER = 1500;
    static constexpr float MIN_GAIN= 0;
    static constexpr float MAX_GAIN= 50.0;
    static constexpr float DEF_GAIN= 10.0;
    static constexpr int DEF_BT_MODE= 0; // Bluetooth Disabled

    TrackerSettings(QObject *parent=nullptr);
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

    int ppmOutPin() const;
    void setPpmOutPin(int value);

    bool invertedPpmOut() const;
    void setInvertedPpmOut(bool value);

    int ppmInPin() const;
    void setPpmInPin(int value);

    bool invertedPpmIn() const;
    void setInvertedPpmIn(bool value);

    int buttonPin() const;
    void setButtonPin(int value);

    bool resetOnWave() const {return _data["rstonwave"].toBool();}
    void setResetOnWave(bool value) {_data["rstonwave"] = value;}

    void gyroOffset(float &x, float &y, float &z);
    void setGyroOffset(float x,float y, float z);

    void accOffset(float &x, float &y, float &z);
    void setAccOffset(float x,float y, float z);

    void magOffset(float &x, float &y, float &z);
    void setMagOffset(float x,float y, float z);

    /*void magSiOffset(float v[]) {memcpy(v,_data["magsioff",9*sizeof(float));}
    void setMagSiOffset(float *v) {memcpy(magsioff,v,9*sizeof(float));}
    QList<float> magsioff;*/
    int count() const {return 22;}

    uint axisRemap() const {return _data["axisremap"].toUInt();}
    void setAxisRemap(uint value);

    uint axisSign() const {return _data["axissign"].toUInt();}
    void setAxisSign(uint value);

    int blueToothMode();
    void setBlueToothMode(int mode);

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
};

#endif // TRACKERSETTINGS_H
