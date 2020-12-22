#include <QDebug>
#include "trackersettings.h"

TrackerSettings::TrackerSettings()
{
    // Defaults
    _data["rll_min"] = MIN_PWM;
    _data["rll_max"] = MAX_PWM;
    _data["rll_gain"] =  (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    _data["rll_cnt"] = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    _data["pan_min"] = MIN_PWM;
    _data["pan_max"] = MAX_PWM;
    _data["pan_gain"] =  (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    _data["pan_cnt"] = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    _data["tlt_min"] = MIN_PWM;
    _data["tlt_max"] = MAX_PWM;
    _data["tlt_gain"] =  (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    _data["tlt_cnt"] = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

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
