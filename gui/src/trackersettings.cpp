#include <QDebug>
#include "trackersettings.h"

TrackerSettings::TrackerSettings(QObject *parent):
    QObject(parent)
{
    // Defaults
    _data["rll_min"] = DEF_MIN_PWM;
    _data["rll_max"] = DEF_MAX_PWM;
    _data["rll_gain"] = DEF_GAIN;
    _data["rll_cnt"] = PPM_CENTER;

    _data["pan_min"] = DEF_MIN_PWM;
    _data["pan_max"] = DEF_MAX_PWM;
    _data["pan_gain"] = DEF_GAIN;
    _data["pan_cnt"] = PPM_CENTER;

    _data["tlt_min"] = DEF_MIN_PWM;
    _data["tlt_max"] = DEF_MAX_PWM;
    _data["tlt_gain"] = DEF_GAIN;
    _data["tlt_cnt"] = PPM_CENTER;

    _data["panch"] = DEF_PAN_CH;
    _data["tltch"] = DEF_TILT_CH;
    _data["rllch"] = DEF_ROLL_CH;

    _data["servoreverse"] = (uint)0x00;

    _data["lppan"] = DEF_LP_PAN;
    _data["lptiltroll"] = DEF_LP_TLTRLL;

    _data["axisremap"] = (uint)AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z);
    _data["axissign"] = (uint)0;

    _data["buttonpin"] = DEF_BUTTON_IN;
    _data["butlngps"] = DEF_BUTTON_LONG_PRESS;
    _data["rstontlt"] = DEF_RESET_ON_TILT;
    _data["ppminpin"] = DEF_PPM_IN;
    _data["ppmoutpin"] = DEF_PPM_OUT;
    _data["ppmoutinvert"] = false;
    _data["ppmfrm"] = DEF_PPM_FRAME;
    _data["ppmsync"] = DEF_PPM_SYNC;
    _data["ppmchcnt"] = DEF_PPM_CHANNELS;

    _data["ppmininvert"] = false;
    _data["btmode"] = (uint)0;
    _data["orient"] = (uint)0;
    _data["rstppm"] = DEF_RST_PPM;

    // PWM defaults
    _data["pwm0"] = DEF_PWM_A0_CH;
    _data["pwm1"] = DEF_PWM_A1_CH;
    _data["pwm2"] = DEF_PWM_A2_CH;
    _data["pwm3"] = DEF_PWM_A3_CH;

    // SBUS Defaults
    _data["sbininv"] = DEF_SBUS_IN_INV;
    _data["sboutinv"] = DEF_SBUS_OUT_INV;
    _data["sbrate"] = DEF_SBUS_RATE;

    // Analog defaults
    _data["an4ch"]  = DEF_ALG_A4_CH;
    _data["an4gain"] = DEF_ALG_GAIN;
    _data["an4off"] = DEF_ALG_OFFSET;

    _data["an5ch"]  = DEF_ALG_A5_CH;
    _data["an5gain"] = DEF_ALG_GAIN;
    _data["an5off"] = DEF_ALG_OFFSET;

    _data["an6ch"]  = DEF_ALG_A6_CH;
    _data["an6gain"] = DEF_ALG_GAIN;
    _data["an6off"] = DEF_ALG_OFFSET;

    _data["an7ch"] = DEF_ALG_A7_CH;
    _data["an7gain"] = DEF_ALG_GAIN;
    _data["an7off"]  = DEF_ALG_OFFSET;

    // AUX defaults
    _data["aux0ch"] = DEF_AUX_CH0;
    _data["aux1ch"] = DEF_AUX_CH1;
    _data["aux2ch"] = DEF_AUX_CH2;
    _data["aux0func"] = DEF_AUX_FUNC;
    _data["aux1func"] = DEF_AUX_FUNC;
    _data["aux3func"] = DEF_AUX_FUNC;

    _data["rotx"] = DEF_BOARD_ROT_X;
    _data["roty"] = DEF_BOARD_ROT_Y;
    _data["rotz"] = DEF_BOARD_ROT_Z;

    _live["btaddr"] = QString("00:00:00:00:00:00");

    // Add All Realtime data Items to Map
    #define DV(DT, NAME, DIV, ROUND) _realtimedata[ #NAME ] = false;
        DATA_VARS
    #undef DV
    #define DA(DT, NAME, SIZE, DIV) _realtimedata[ #NAME ] = false;
        DATA_ARRAYS
    #undef DA
    _devicerealtimedata = _realtimedata;

}

int TrackerSettings::Rll_min() const
{
    return _data["rll_min"].toInt();
}

void TrackerSettings::setRll_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["rll_min"] = value;
}

int TrackerSettings::Rll_max() const
{
    return _data["rll_max"].toInt();
}

void TrackerSettings::setRll_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["rll_max"] = value;
}

float TrackerSettings::Rll_gain() const
{
    return _data["rll_gain"].toFloat();
}

void TrackerSettings::setRll_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["rll_gain"] = QString::number(value,'g',3);
}

int TrackerSettings::Rll_cnt() const
{
    return _data["rll_cnt"].toInt();
}

void TrackerSettings::setRll_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    _data["rll_cnt"] = value;
}

int TrackerSettings::Pan_min() const
{
    return _data["pan_min"].toInt();
}

void TrackerSettings::setPan_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["pan_min"] = value;
}

int TrackerSettings::Pan_max() const
{
    return _data["pan_max"].toInt();
}

void TrackerSettings::setPan_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["pan_max"] = value;
}

float TrackerSettings::Pan_gain() const
{
    return _data["pan_gain"].toFloat();
}

void TrackerSettings::setPan_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["pan_gain"] = QString::number(value,'g',3);
}

int TrackerSettings::Pan_cnt() const
{
    return _data["pan_cnt"].toInt();
}

void TrackerSettings::setPan_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    _data["pan_cnt"] = value;
}

int TrackerSettings::Tlt_min() const
{
    return _data["tlt_min"].toInt();
}

void TrackerSettings::setTlt_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["tlt_min"] = value;
}

int TrackerSettings::Tlt_max() const
{
    return _data["tlt_max"].toInt();
}

void TrackerSettings::setTlt_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    _data["tlt_max"] = value;
}

float TrackerSettings::Tlt_gain() const
{
    return _data["tlt_gain"].toFloat();
}

void TrackerSettings::setTlt_gain(float value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["tlt_gain"] = QString::number(value,'g',3);
}

int TrackerSettings::Tlt_cnt() const
{
    return _data["tlt_cnt"].toInt();
}

void TrackerSettings::setTlt_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    _data["tlt_cnt"] = value;
}

int TrackerSettings::lpTiltRoll() const
{
    return _data["lptiltroll"].toInt();
}

void TrackerSettings::setLPTiltRoll(int value)
{
    _data["lptiltroll"] = value;
}

int TrackerSettings::lpPan() const
{
    return _data["lppan"].toInt();
}

void TrackerSettings::setLPPan(int value)
{
    _data["lppan"] = value;
}

int TrackerSettings::gyroWeightTiltRoll() const
{
    return _data["gyroweighttiltroll"].toInt();
}

void TrackerSettings::setGyroWeightTiltRoll(int value)
{
    _data["gyroweighttiltroll"] = value;
}

int TrackerSettings::gyroWeightPan() const
{
    return _data["gyroweightpan"].toInt();
}

void TrackerSettings::setGyroWeightPan(int value)
{
    _data["gyroweightpan"] = value;
}

char TrackerSettings::servoReverse() const
{
    return _data["servoreverse"].toUInt();
}

void TrackerSettings::setServoreverse(char value)
{
    _data["servoreverse"] = (uint)value;
}

void TrackerSettings::setRollReversed(bool value)
{
    if(value)
        _data["servoreverse"] = _data["servoreverse"].toUInt() | HT_ROLL_REVERSE_BIT;
    else
        _data["servoreverse"] = _data["servoreverse"].toUInt() & (HT_ROLL_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setPanReversed(bool value)
{
    if(value)
        _data["servoreverse"] = _data["servoreverse"].toUInt() | HT_PAN_REVERSE_BIT;
    else
        _data["servoreverse"] = _data["servoreverse"].toUInt() & (HT_PAN_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setTiltReversed(bool value)
{
    if(value)
        _data["servoreverse"] = _data["servoreverse"].toUInt() | HT_TILT_REVERSE_BIT;
    else
        _data["servoreverse"] = _data["servoreverse"].toUInt() & (HT_TILT_REVERSE_BIT ^ 0xFFFF);
}

bool TrackerSettings::isRollReversed()
{
    return (_data["servoreverse"].toUInt() & HT_ROLL_REVERSE_BIT);
}

bool TrackerSettings::isTiltReversed()
{
    return (_data["servoreverse"].toUInt() & HT_TILT_REVERSE_BIT);
}

bool TrackerSettings::isPanReversed()
{
    return (_data["servoreverse"].toUInt() & HT_PAN_REVERSE_BIT);
}

int TrackerSettings::panCh() const
{
    return _data["panch"].toUInt();
}

void TrackerSettings::setPanCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        _data["panch"] = value;
}

int TrackerSettings::tiltCh() const
{
    return _data["tltch"].toUInt();
}

void TrackerSettings::setTiltCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        _data["tltch"] = value;
}

int TrackerSettings::rollCh() const
{
    return _data["rllch"].toUInt();
}

void TrackerSettings::setRollCh(int value)
{
    if((value > 0 && value < 17) || value == -1)
        _data["rllch"] = value;
}

int TrackerSettings::ppmOutPin() const
{
    return _data["ppmoutpin"].toInt();
}

void TrackerSettings::setPpmOutPin(int value)
{
    _data["ppmoutpin"] = value;
}

bool TrackerSettings::invertedPpmOut() const
{
    return _data["ppmoutinvert"].toBool();
}

void TrackerSettings::setInvertedPpmOut(bool value)
{
    _data["ppmoutinvert"] = value;
}

int TrackerSettings::ppmInPin() const
{
    return _data["ppminpin"].toInt();
}

void TrackerSettings::setPpmInPin(int value)
{
    _data["ppminpin"] = value;
}

bool TrackerSettings::invertedPpmIn() const
{
    return _data["ppmininvert"].toBool();
}

void TrackerSettings::setInvertedPpmIn(bool value)
{
    _data["ppmininvert"] = value;
}

int TrackerSettings::buttonPin() const
{
    return _data["buttonpin"].toInt();
}

void TrackerSettings::setButtonPin(int value)
{
    _data["buttonpin"] = value;
}

int TrackerSettings::resetCntPPM() const
{
    return _data["rstppm"].toInt();
}

void TrackerSettings::setResetCntPPM(int value)
{
    if((value >= 1 && value <= 16) || value == -1)
        _data["rstppm"] = value;
}

void TrackerSettings::orientation(int &x,int &y,int &z)
{
    x = _data["rotx"].toInt();
    y = _data["roty"].toInt();
    z = _data["rotz"].toInt();
    return;
}

void TrackerSettings::setOrientation(int x,int y,int z)
{
    _data["rotx"] = x;
    _data["roty"] = y;
    _data["rotz"] = z;
}

void TrackerSettings::gyroOffset(float &x, float &y, float &z)
{
    x=_data["gyrxoff"].toFloat();
    y=_data["gyryoff"].toFloat();
    z=_data["gyrzoff"].toFloat();
}

void TrackerSettings::setGyroOffset(float x, float y, float z)
{
    _data["gyrxoff"]=QString::number(x,'g',3);
    _data["gyryoff"]=QString::number(y,'g',3);
    _data["gyrzoff"]=QString::number(z,'g',3);
}

void TrackerSettings::accOffset(float &x, float &y, float &z)
{
    x=_data["accxoff"].toFloat();
    y=_data["accyoff"].toFloat();
    z=_data["acczoff"].toFloat();
}

void TrackerSettings::setAccOffset(float x, float y, float z)
{
    _data["accxoff"]=QString::number(x,'g',3);
    _data["accyoff"]=QString::number(y,'g',3);
    _data["acczoff"]=QString::number(z,'g',3);}

void TrackerSettings::magOffset(float &x, float &y, float &z)
{
    x=_data["magxoff"].toFloat();
    y=_data["magyoff"].toFloat();
    z=_data["magzoff"].toFloat();
}

void TrackerSettings::setMagOffset(float x, float y, float z)
{
    _data["magxoff"]=QString::number(x,'g',3);
    _data["magyoff"]=QString::number(y,'g',3);
    _data["magzoff"]=QString::number(z,'g',3);
}

void TrackerSettings::setAnalog4Gain(float gain)
{
    _data["an4gain"]=QString::number(gain,'g',4);
}

void TrackerSettings::setAnalog5Gain(float gain)
{
    _data["an5gain"]=QString::number(gain,'g',4);
}

void TrackerSettings::setAnalog6Gain(float gain)
{
    _data["an6gain"]=QString::number(gain,'g',4);
}

void TrackerSettings::setAnalog7Gain(float gain)
{
    _data["an7gain"] = QString::number(gain,'g',4);
}

void TrackerSettings::setAxisRemap(uint value)
{
    _data["axisremap"] = (uint)(value & 0x3F);
}

void TrackerSettings::setAxisSign(uint value)
{
    _data["axissign"] = (uint)(value & 0x07);
}

int TrackerSettings::blueToothMode()
{
    return _data["btmode"].toInt();
}

void TrackerSettings::setBlueToothMode(int mode) {
    if(mode >= 0 && mode <= 3)
        _data["btmode"] = mode;
}

QString TrackerSettings::blueToothAddress()
{
    return _live["btaddr"].toString();
}

void TrackerSettings::storeSettings(QSettings *settings)
{
    settings->clear();
    QMapIterator<QString, QVariant> i(_data);
    while (i.hasNext()) {
        i.next();
        if(i.key() == "Cmd")
            continue;
        settings->setValue(i.key(),i.value());
    }
}

void TrackerSettings::loadSettings(QSettings *settings)
{
    QStringList keys = settings->allKeys();

    foreach(QString key,keys) {
        _data[key] = settings->value(key);
    }
}

QVariantMap TrackerSettings::allData()
{
    return _data;
}

void TrackerSettings::setAllData(const QVariantMap &data)
{
    QStringList keys = data.keys();
    foreach(QString key,keys) {
        _data[key] = data.value(key);
        _devicedata[key] = data.value(key);
    }
}

QVariantMap TrackerSettings::changedData()
{
    QVariantMap changed;
    QStringList keys = _data.keys();
    foreach(QString key,keys) {
        // Current Data != Data on Device
        if(_data[key] != _devicedata[key]) {
            changed[key] = _data[key];
        }
    }
    return changed;
}

void TrackerSettings::setDataMatched()
{
    // Update device data to current data
    _devicedata = _data;
}

QVariant TrackerSettings::liveData(const QString &name)
{
    return _live[name];
}

void TrackerSettings::setLiveData(const QString &name, const QVariant &live)
{
    QVariantMap map;
    map[name] = live;
    setLiveDataMap(map);
}

QVariantMap TrackerSettings::liveDataMap()
{
    return _live;
}

void TrackerSettings::setLiveDataMap(const QVariantMap &livelist,bool reset)
{
    if(reset)
        _live.clear();

    // Update List
    _live = livelist;

    // Emit if a value has been updated
    bool ge=false,ae=false,me=false,oe=false,ooe=false,ppm=false;
    QMapIterator<QString, QVariant> i(livelist);
    while (i.hasNext()) {
        i.next();
        QString key = i.key();
        if((key == "gyrox" || key == "gyroy" || key == "gyroz") && !ge) {
            emit(rawGyroChanged(_live["gyrox"].toFloat(),
                                _live["gyroy"].toFloat(),
                                _live["gyroz"].toFloat()));
            ge = true;
        }
        if((key == "accx" || key == "accy" || key == "accz") && !ae) {
            emit(rawAccelChanged(_live["gyrox"].toFloat(),
                                _live["gyroy"].toFloat(),
                                _live["gyroz"].toFloat()));
            ae = true;
        }
        if((key == "magx" || key == "magy" || key == "magz") && !me) {
            emit(rawMagChanged(_live["magx"].toFloat(),
                                _live["magy"].toFloat(),
                                _live["magz"].toFloat()));
            me = true;
        }
        if((key == "tilt" || key == "roll" || key == "pan") && !oe) {
            emit(rawOrientChanged(_live["tilt"].toFloat(),
                                _live["roll"].toFloat(),
                                _live["pan"].toFloat()));
            oe = true;
        }
        if((key == "tiltoff" || key == "rolloff" || key == "panoff") && !ooe) {
            emit(offOrientChanged(_live["tiltoff"].toFloat(),
                                _live["rolloff"].toFloat(),
                                _live["panoff"].toFloat()));
            ooe = true;
        }
        if((key == "panout" || key == "tiltout" || key == "rollout") && !ppm) {
            emit(ppmOutChanged(_live["tiltout"].toUInt(),
                                _live["rollout"].toUInt(),
                                _live["panout"].toUInt()));
            ppm = true;
        }
        if(key == "btrmt") {
            if(_live["btrmt"] != "")
                emit(bleAddressDiscovered(_live["btrmt"].toString()));
        }
    }

    // Notify data has changed
    emit liveDataChanged();
}

void TrackerSettings::setHardware(QString vers, QString hard)
{
    _data["Vers"] = vers;
    _data["Hard"] = hard;
}

void TrackerSettings::setSoftIronOffsets(float soo[3][3])
{
    for(int i=0;i < 3; i++){
        for(int j=0;j<3;j++) {
            QString element = QString("so%1%2").arg(i).arg(j);
            _data[element] = QString::number(soo[i][j],'g',3);
        }
    }
}

// Sets all data items to diabled
void TrackerSettings::clearDataItems()
{
    QMapIterator<QString, bool> i(_realtimedata);
    while (i.hasNext()) {
        i.next();
        _realtimedata[i.key()] = false;
    }
    setDataItemsMatched();
}

// Add/remove a single item to be sent
void TrackerSettings::setDataItemSend(const QString &itm, const bool &enabled)
{
    if(_realtimedata.contains(itm)) {
        if(_realtimedata[itm] != enabled) {
            _realtimedata[itm] = enabled;
            emit requestedDataItemChanged();
        }
    }
}

// Add/remove multiple items
void TrackerSettings::setDataItemSend(QMap<QString, bool> items)
{
    QMap<QString, bool>::iterator i;
    for(i = items.begin(); i != items.end(); ++i) {
        setDataItemSend(i.key(),i.value());
    }
}

// Returns the differences between currently sending data items
// and items which need to be sent/removed
QMap<QString, bool> TrackerSettings::getDataItemsDiff()
{
    QMap<QString, bool> diffs;
    QMapIterator<QString, bool> i(_realtimedata);
    while (i.hasNext()) {
        i.next();
        if(_devicerealtimedata[i.key()] != i.value()) {
            diffs[i.key()] = i.value();
        }
    }
    return diffs;
}

// Returns the currently sending data items

QMap<QString, bool> TrackerSettings::getDataItems()
{
  return _realtimedata;
}

QStringList TrackerSettings::allDataItems()
{
  QStringList rv;
#define DV(DT, NAME, DIV, ROUND) rv.append(#NAME);
    DATA_VARS
#undef DV

#define DA(DT, NAME, SIZE, DIV) \
  for(int i=0; i < SIZE; i++) {\
    rv.append(QString(#NAME "[%1]").arg(i));\
  }
    DATA_ARRAYS
#undef DA

  return rv;

}

