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

// Arrays to be sent back to GUI if enabled
#define DATA_ARRAYS\
    DA(u16, chout, 16, 1)\
    DA(u16, btch, 8, 1)\
    DA(u16, ppmch, 16, 1)\
    DA(u16, sbusch, 16, 1)\
    DA(flt, quat,4, 1)\
    DA(chr, btaddr,18, 20) \
    DA(chr, btrmt,18, 10)

class TrackerSettings : public QObject
{    
    Q_OBJECT
public:
    enum {BTDISABLE,BTPARAHEAD,BTPARARMT};

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
    static constexpr bool DEF_BUTTON_LONG_PRESS = true;
    static constexpr bool DEF_RESET_ON_TILT = false;
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
    static constexpr int DEF_TILT_CH = 1;
    static constexpr int DEF_ROLL_CH = 2;
    static constexpr int DEF_PAN_CH = 3;
    static constexpr int DEF_ALERT_CH = -1;
    static constexpr int DEF_PWM_A0_CH = -1;
    static constexpr int DEF_PWM_A1_CH = -1;
    static constexpr int DEF_PWM_A2_CH = -1;
    static constexpr int DEF_PWM_A3_CH = -1;
    static constexpr bool DEF_SBUS_IN_INV = false;
    static constexpr bool DEF_SBUS_OUT_INV = false;
    static constexpr int DEF_SBUS_RATE = 60;
    static constexpr int DEF_ALG_A4_CH = -1;
    static constexpr int DEF_ALG_A5_CH = -1;
    static constexpr int DEF_ALG_A6_CH = -1;
    static constexpr int DEF_ALG_A7_CH = -1;
    static constexpr float DEF_ALG_GAIN = 310.00f;
    static constexpr int DEF_ALG_OFFSET = 0;
    static constexpr int DEF_AUX_CH0 = -1;
    static constexpr int DEF_AUX_CH1 = -1;
    static constexpr int DEF_AUX_CH2 = -1;
    static constexpr int DEF_AUX_FUNC = 0;
    static constexpr float DEF_KP = 0.2f;
    static constexpr float DEF_KI = 0.01f;

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

    int alertCh() const;
    void setAlertCh(int value);

    int ppmOutPin() const;
    void setPpmOutPin(int value);

    bool invertedPpmOut() const;
    void setInvertedPpmOut(bool value);

    uint ppmFrame() {return _data["ppmfrm"].toInt();}
    void setPPMFrame(uint v) { if(v >= PPM_MIN_FRAME  && v <= PPM_MAX_FRAME) _data["ppmfrm"] = v;}

    uint ppmSync() {return _data["ppmsync"].toInt();}
    void setPPMSync(uint v) { if(v >= PPM_MIN_SYNC && v <= PPM_MAX_SYNC)_data["ppmsync"] = v;}

    int ppmChCount() {return _data["ppmchcnt"].toInt();}
    void setPpmChCount(int v) { if(v > 0 && v<17) _data["ppmchcnt"] = v;}

    int ppmInPin() const;
    void setPpmInPin(int value);

    bool invertedPpmIn() const;
    void setInvertedPpmIn(bool value);

    void setInvertedSBUSIn(bool v) {_data["sbininv"] = v;}
    bool invertedSBUSIn() {return _data["sbininv"].toBool();}

    void setInvertedSBUSOut(bool v) {_data["sboutinv"] = v;}
    bool invertedSBUSOut() {return _data["sboutinv"].toBool();}

    void setSBUSRate(uint rate) {_data["sbrate"] = rate;}
    uint SBUSRate() { return _data["sbrate"].toUInt();}

    int buttonPin() const;
    void setButtonPin(int value);

    void setButtonPressMode(bool lngpresmd) {_data["butlngps"] = lngpresmd;} // True = Enable/Disable output on long press
    bool buttonPressMode() {return _data["butlngps"].toBool();}

    void setResetOnTilt(bool r) {_data["rstontlt"] = r;}
    bool resetOnTiltMode() {return _data["rstontlt"].toBool();}

    int resetCntPPM() const;
    void setResetCntPPM(int value);

    bool resetOnWave() const {return _data["rstonwave"].toBool();}
    void setResetOnWave(bool value) {_data["rstonwave"] = value;}

    void orientation(int &x,int &y,int &z);
    void setOrientation(int x,int y,int z);

    void gyroOffset(float &x, float &y, float &z);
    void setGyroOffset(float x,float y, float z);

    void accOffset(float &x, float &y, float &z);
    void setAccOffset(float x,float y, float z);

    void magOffset(float &x, float &y, float &z);
    void setMagOffset(float x,float y, float z);

    // Analogs    
    void setAnalog4Ch(int channel) {_data["an4ch"] = channel;}
    void setAnalog4Gain(float gain);
    void setAnalog4Offset(int offset) {_data["an4off"] = offset;}
    int analog4Ch() {return _data["an4ch"].toInt() ;}
    float analog4Gain() {return _data["an4gain"].toFloat();}
    int analog4Offset() {return _data["an4off"].toFloat();}

    void setAnalog5Ch(int channel) {_data["an5ch"] = channel;}
    void setAnalog5Gain(float gain);
    void setAnalog5Offset(int offset) {_data["an5off"] = offset;}
    int analog5Ch() {return _data["an5ch"].toInt() ;}
    float analog5Gain() {return _data["an5gain"].toFloat();}
    int analog5Offset() {return _data["an5off"].toFloat();}

    void setAnalog6Ch(int channel) {_data["an6ch"] = channel;}
    void setAnalog6Gain(float gain);
    void setAnalog6Offset(int offset) {_data["an6off"] = offset;}
    int analog6Ch() {return _data["an6ch"].toInt() ;}
    float analog6Gain() {return _data["an6gain"].toFloat();}
    int analog6Offset() {return _data["an6off"].toFloat();}

    void setAnalog7Ch(int channel) {_data["an7ch"] = channel;}
    void setAnalog7Gain(float gain);
    void setAnalog7Offset(int offset) {_data["an7off"] = offset;}
    int analog7Ch() {return _data["an7ch"].toInt();}
    float analog7Gain() {return _data["an7gain"].toFloat();}
    int analog7Offset() {return _data["an7off"].toFloat();}

    // Aux Functions
    void setAuxFunc0Ch(int channel) {_data["aux0ch"] = channel;}
    void setAuxFunc1Ch(int channel){_data["aux1ch"] = channel;}
    void setAuxFunc2Ch(int channel){_data["aux2ch"] = channel;}
    void setAuxFunc0(int funct) {_data["aux0func"] = funct;}
    void setAuxFunc1(int funct) {_data["aux1func"] = funct;}
    void setAuxFunc2(int funct) {_data["aux2func"] = funct;}
    int auxFunc0Ch() {return _data["aux0ch"].toInt();}
    int auxFunc1Ch() {return _data["aux1ch"].toInt();}
    int auxFunc2Ch() {return _data["aux2ch"].toInt();}
    int auxFunc0() {return _data["aux0func"].toInt();}
    int auxFunc1() {return _data["aux1func"].toInt();}
    int auxFunc2() {return _data["aux2func"].toInt();}

    // IMU Functions
    float Kp() {return _data["Kp"].toFloat();}
    float Ki() {return _data["Ki"].toFloat();}
    void setKp(float value) {_data["Kp"] = QString::number(value,'g',4);}
    void setKi(float value) {_data["Ki"] = QString::number(value,'g',4);}

    // PWM Functions
    int pwmCh(int ch) {return _data[QString("pwm%1").arg(ch)].toInt();}
    void setPWMCh(int ch, int val) {_data[QString("pwm%1").arg(ch)] = val;}

    int count() const {return 22;} //BNO, how many values should there be

    uint axisRemap() const {return _data["axisremap"].toUInt();}
    void setAxisRemap(uint value);        

    uint axisSign() const {return _data["axissign"].toUInt();}
    void setAxisSign(uint value);

    int blueToothMode();
    void setBlueToothMode(int mode);
    QString blueToothAddress();
    bool blueToothConnected() {return _live["btcon"].toBool();}
    bool tiltRollPanEnabled() {return _live["trpenabled"].toBool();}

    void setPairedBTAddress(QString addr="") {_data["btpair"] = addr;}
    QString pairedBTAddress() {return _data["btpair"].toString();}

    QString PPMInString() {return _live["ppmin"].toString();}
    QString PPMOutString() {return _live["ppmout"].toString();}

    void storeSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

    QVariantMap allData();
    void setAllData(const QVariantMap &data);

    QVariantMap changedData();
    void setDataMatched();

    QVariant liveData(const QString &name);
    void setLiveData(const QString &name, const QVariant &live);
    void clearLiveData() {_live.clear();}

    QVariantMap liveDataMap();
    void setLiveDataMap(const QVariantMap &livelist, bool clear=false);

    QString hardware() {return _data["Hard"].toString();}
    QString fwVersion() {return _data["Vers"].toString();}
    void setHardware(QString vers,QString hard, QString git="");

    void setSoftIronOffsets(float soo[3][3]);

    void clear() {_data.clear();_live.clear();}

    // Realtime data requested from the board
    // Keeps track of them in _data2send list
    void clearDataItems();
    void setDataItemSend(const QString &itm, const bool &enabled);
    void setDataItemSend(QMap<QString,bool> items);
    QMap<QString, bool> getDataItemsDiff();
    void setDataItemsMatched() {_devicerealtimedata = _realtimedata;}
    // Gets all currently sending data items
    QMap<QString, bool> getDataItems();    
    QStringList allDataItems();

signals:
    void rawGyroChanged(float x, float y, float z);
    void rawAccelChanged(float x, float y, float z);
    void rawMagChanged(float x, float y, float z);
    void rawOrientChanged(float t, float r, float p);
    void offOrientChanged(float t, float r, float p);
    void bleAddressDiscovered(QString);
    void ppmOutChanged(int t, int r, int p);
    void liveDataChanged();
    void requestedDataItemChanged();

private:
    QVariantMap _data; // Data in GUI
    QVariantMap _devicedata; // Stored Data on Device
    QVariantMap _live; // Live Data (Realtime)
    QMap<QString, bool> _realtimedata;
    QMap<QString, bool> _devicerealtimedata;
    QStringList bleAddresses;
};

#endif // TRACKERSETTINGS_H
