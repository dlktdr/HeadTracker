#include <QDebug>
#include "trackersettings.h"

TrackerSettings::TrackerSettings(QObject *parent):
    QObject(parent)
{
    // Defaults
    _data["rll_min"] = MIN_PWM;
    _data["rll_max"] = MAX_PWM;
    _data["rll_gain"] = DEF_GAIN;
    _data["rll_cnt"] = DEF_CENTER;

    _data["pan_min"] = MIN_PWM;
    _data["pan_max"] = MAX_PWM;
    _data["pan_gain"] = DEF_GAIN;
    _data["pan_cnt"] = DEF_CENTER;

    _data["tlt_min"] = MIN_PWM;
    _data["tlt_max"] = MAX_PWM;
    _data["tlt_gain"] = DEF_GAIN;
    _data["tlt_cnt"] = DEF_CENTER;

    _data["panch"] = (uint)6;
    _data["tltch"] = (uint)7;
    _data["rllch"] = (uint)8;

    _data["servoreverse"] = (uint)0x00;

    _data["lppan"] = (uint)10;
    _data["lptiltroll"] = (uint)20;

    _data["gyroweightpan"] = (uint)30;
    _data["gyroweighttiltroll"] = (uint)40;

    _data["axisremap"] = (uint)AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z);
    _data["axissign"] = (uint)0;
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

int TrackerSettings::Rll_gain() const
{
    return _data["rll_gain"].toInt();
}

void TrackerSettings::setRll_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["rll_gain"] = value;
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

int TrackerSettings::Pan_gain() const
{
    return _data["pan_gain"].toInt();
}

void TrackerSettings::setPan_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["pan_gain"] = value;
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

int TrackerSettings::Tlt_gain() const
{
    return _data["tlt_gain"].toInt();
}

void TrackerSettings::setTlt_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    _data["tlt_gain"] = value;
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

uint TrackerSettings::panCh() const
{
    return _data["panch"].toUInt();
}

void TrackerSettings::setPanCh(uint value)
{
    if(value > 0 && value < 17)
        _data["panch"] = (uint)value;
}

uint TrackerSettings::tiltCh() const
{
    return _data["tltch"].toUInt();
}

void TrackerSettings::setTiltCh(uint value)
{
    if(value > 0 && value < 17)
        _data["tltch"] = (uint)value;
}

uint TrackerSettings::rollCh() const
{
    return _data["rllch"].toUInt();
}

void TrackerSettings::setRollCh(uint value)
{
    if(value > 0 && value < 17)
        _data["rllch"] = (uint)value;
}

void TrackerSettings::gyroOffset(float &x, float &y, float &z)
{
    x=_data["gyrxoff"].toFloat();y=_data["gyryoff"].toFloat();z=_data["gyrzoff"].toFloat();
}

void TrackerSettings::setGyroOffset(float x, float y, float z)
{
    _data["gyrxoff"]=x;_data["gyryoff"]=y;_data["gyrzoff"]=z;
}

void TrackerSettings::accOffset(float &x, float &y, float &z)
{
    x=_data["accxoff"].toFloat();y=_data["accyoff"].toFloat();z=_data["acczoff"].toFloat();
}

void TrackerSettings::setAccOffset(float x, float y, float z)
{
    _data["accxoff"]=x;_data["accyoff"]=y;_data["acczoff"]=z;}

void TrackerSettings::magOffset(float &x, float &y, float &z)
{
    x=_data["magxoff"].toFloat();y=_data["magyoff"].toFloat();z=_data["magzoff"].toFloat();
}

void TrackerSettings::setMagOffset(float x, float y, float z)
{
    _data["magxoff"]=x;_data["magyoff"]=y;_data["magzoff"]=z;
}

void TrackerSettings::setAxisRemap(uint value)
{
    _data["axisremap"] = (uint)(value & 0x3F);
}

void TrackerSettings::setAxisSign(uint value)
{        
    _data["axissign"] = (uint)(value & 0x07);
}

void TrackerSettings::storeSettings(QSettings *settings)
{
    settings->clear();
    QMapIterator<QString, QVariant> i(_data);
    while (i.hasNext()) {
        i.next();
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

void TrackerSettings::setAllData(const QVariantMap &data)
{
    QStringList keys = data.keys();
    foreach(QString key,keys) {
        _data[key] = data.value(key);
    }
}

QVariant TrackerSettings::getLiveData(const QString &name)
{
    return _live[name];
}

void TrackerSettings::setLiveData(const QString &name, const QVariant &live)
{
    QVariantMap map;
    map[name] = live;
    setLiveDataMap(map);
}

QVariantMap TrackerSettings::getLiveDataMap()
{
    return _live;
}

void TrackerSettings::setLiveDataMap(const QVariantMap &livelist,bool reset)
{
    if(reset)
        _live.clear();

    // Update List
    _live.insert(livelist);

    // Emit if a value has been updated
    bool ge=false,ae=false,me=false,oe=false,ooe=false,ppm=false;
    QMapIterator<QString, QVariant> i(livelist);
    while (i.hasNext()) {
        i.next();        
        if((i.key() == "gyrox" || i.key() == "gyroy" || i.key() == "gyroz") && !ge) {
            emit(rawGyroChanged(_live["gyrox"].toFloat(),
                                _live["gyroy"].toFloat(),
                                _live["gyroz"].toFloat()));
            ge = true;
        }
        if((i.key() == "accx" || i.key() == "accy" || i.key() == "accz") && !ae) {
            emit(rawAccelChanged(_live["gyrox"].toFloat(),
                                _live["gyroy"].toFloat(),
                                _live["gyroz"].toFloat()));
            ae = true;
        }
        if((i.key() == "magx" || i.key() == "magy" || i.key() == "magz") && !me) {
            emit(rawMagChanged(_live["magx"].toFloat(),
                                _live["magy"].toFloat(),
                                _live["magz"].toFloat()));
            me = true;
        }
        if((i.key() == "tilt" || i.key() == "roll" || i.key() == "pan") && !oe) {
            emit(rawOrientChanged(_live["tilt"].toFloat(),
                                _live["roll"].toFloat(),
                                _live["pan"].toFloat()));
            oe = true;
        }
        if((i.key() == "tiltoff" || i.key() == "rolloff" || i.key() == "panoff") && !ooe) {
            emit(offOrientChanged(_live["tiltoff"].toFloat(),
                                _live["rolloff"].toFloat(),
                                _live["panoff"].toFloat()));
            ooe = true;
        }
        if((i.key() == "panout" || i.key() == "tiltout" || i.key() == "rollout") && !ppm) {
            emit(ppmOutChanged(_live["tiltout"].toUInt(),
                                _live["rollout"].toUInt(),
                                _live["panout"].toUInt()));
            ppm = true;
        }
    }
}

void TrackerSettings::setHardware(QString vers, QString hard)
{
    _data["Vers"] = vers;
    _data["Hard"] = hard;
}
