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
    enum {BTDISABLE,BTMASTER,BTREMOTE};

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
    static constexpr int DEF_BUTTON_IN = 2; // Chosen because it's beside ground
    static constexpr int DEF_PPM_OUT = 10; // Random choice
    static constexpr uint16_t DEF_PPM_FRAME = 22500;
    static constexpr uint16_t PPM_MAX_FRAME = 40000;
    static constexpr uint16_t PPM_MIN_FRAME = 12500;
    static constexpr uint16_t PPM_MIN_FRAMESYNC = 4000; // Not adjustable
    static constexpr int DEF_PPM_SYNC=300;
    static constexpr int PPM_MAX_SYNC=800;
    static constexpr int PPM_MIN_SYNC=100;
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

    int panCh() const;
    void setPanCh(int value);

    int tiltCh() const;
    void setTiltCh(int value);

    int rollCh() const;
    void setRollCh(int value);

    int ppmOutPin() const;
    void setPpmOutPin(int value);

    bool invertedPpmOut() const;
    void setInvertedPpmOut(bool value);

    uint ppmFrame() {return _data["ppmfrm"].toInt();}
    void setPPMFrame(uint v) { if(v >= PPM_MIN_FRAME  && v <= PPM_MAX_FRAME) _data["ppmfrm"] = v;}

    uint ppmSync() {return _data["ppmsync"].toInt();}
    void setPPMSync(uint v) { if(v >= PPM_MIN_SYNC && v <= PPM_MAX_SYNC)_data["ppmsync"] = v;}

    int ppmChCount() {return _data["ppmchcnt"].toInt();}
    void setPpmChCount(int v) { if(v > 3 && v<17) _data["ppmchcnt"] = v;}

    int ppmInPin() const;
    void setPpmInPin(int value);

    bool invertedPpmIn() const;
    void setInvertedPpmIn(bool value);

    int buttonPin() const;
    void setButtonPin(int value);

    int resetCntPPM() const;
    void setResetCntPPM(int value);

    bool resetOnWave() const {return _data["rstonwave"].toBool();}
    void setResetOnWave(bool value) {_data["rstonwave"] = value;}

    uint orientation();
    void setOrientation(uint value);

    void gyroOffset(float &x, float &y, float &z);
    void setGyroOffset(float x,float y, float z);

    void accOffset(float &x, float &y, float &z);
    void setAccOffset(float x,float y, float z);

    void magOffset(float &x, float &y, float &z);
    void setMagOffset(float x,float y, float z);

    int count() const {return 22;} //BNO, how many values should there be

    uint axisRemap() const {return _data["axisremap"].toUInt();}
    void setAxisRemap(uint value);

    uint axisSign() const {return _data["axissign"].toUInt();}
    void setAxisSign(uint value);

    int blueToothMode();
    void setBlueToothMode(int mode);
    QString blueToothAddress();
    bool blueToothConnected() {return _live["btcon"].toBool();}
    QString PPMInString() {return _live["ppmin"].toString();}

    void storeSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

    QVariantMap allData();
    void setAllData(const QVariantMap &data);

    QVariantMap changedData();
    void setDataMatched();

    QVariant liveData(const QString &name);
    void setLiveData(const QString &name, const QVariant &live);

    QVariantMap liveDataMap();
    void setLiveDataMap(const QVariantMap &livelist, bool clear=false);

    QString hardware() {return _data["Hard"].toString();}
    QString fwVersion() {return _data["Vers"].toString();}
    void setHardware(QString vers,QString hard);

    void setSoftIronOffsets(float soo[3][3]);

    void clear() {_data.clear();_live.clear();}

signals:
    void rawGyroChanged(float x, float y, float z);
    void rawAccelChanged(float x, float y, float z);
    void rawMagChanged(float x, float y, float z);
    void rawOrientChanged(float t, float r, float p);
    void offOrientChanged(float t, float r, float p);
    void ppmOutChanged(int t, int r, int p);

private:
    QVariantMap _data; // Data in GUI
    QVariantMap _devicedata; // Stored Data on Device
    QVariantMap _live; // Live Data
};

#endif // TRACKERSETTINGS_H
