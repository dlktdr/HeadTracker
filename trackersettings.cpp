#include <QDebug>
#include "trackersettings.h"

TrackerSettings::TrackerSettings()
{
    // Defaults
    rll_min = MIN_PWM;
    rll_max = MAX_PWM;
    rll_gain = (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    rll_cnt= (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    pan_min = MIN_PWM;
    pan_max = MAX_PWM;
    pan_gain = (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    pan_cnt = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    tlt_min = MIN_PWM;
    tlt_max = MAX_PWM;
    tlt_gain = (MAX_GAIN-MIN_GAIN)/2+MIN_GAIN;
    tlt_cnt = (MAX_PWM-MIN_PWM)/2 + MIN_PWM;

    panch = 6;
    tiltch = 7;
    rollch = 8;

    servoreverse = 0x03;

    lppan = 10;
    lptiltroll = 20;
    gyroweightpan = 30;
    gyroweighttiltroll = 40;
}

int TrackerSettings::Rll_min() const
{
    return rll_min;
}

void TrackerSettings::setRll_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    rll_min = value;
}

int TrackerSettings::Rll_max() const
{
    return rll_max;
}

void TrackerSettings::setRll_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    rll_max = value;
}

int TrackerSettings::Rll_gain() const
{
    return rll_gain;
}

void TrackerSettings::setRll_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    rll_gain = value;
}

int TrackerSettings::Rll_cnt() const
{
    return rll_cnt;
}

void TrackerSettings::setRll_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    rll_cnt = value;
}

int TrackerSettings::Pan_min() const
{
    return pan_min;
}

void TrackerSettings::setPan_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    pan_min = value;
}

int TrackerSettings::Pan_max() const
{
    return pan_max;
}

void TrackerSettings::setPan_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    pan_max = value;
}

int TrackerSettings::Pan_gain() const
{
    return pan_gain;
}

void TrackerSettings::setPan_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    pan_gain = value;
}

int TrackerSettings::Pan_cnt() const
{
    return pan_cnt;
}

void TrackerSettings::setPan_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    pan_cnt = value;
}

int TrackerSettings::Tlt_min() const
{
    return tlt_min;
}

void TrackerSettings::setTlt_min(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    tlt_min = value;
}

int TrackerSettings::Tlt_max() const
{
    return tlt_max;
}

void TrackerSettings::setTlt_max(int value)
{
    if(value < MIN_PWM)
        value = MIN_PWM;
    if(value > MAX_PWM)
        value = MAX_PWM;
    tlt_max = value;
}

int TrackerSettings::Tlt_gain() const
{
    return tlt_gain;
}

void TrackerSettings::setTlt_gain(int value)
{
    if(value < MIN_GAIN)
        value = MIN_GAIN;
    if(value > MAX_GAIN)
        value = MAX_GAIN;
    tlt_gain = value;
}

int TrackerSettings::Tlt_cnt() const
{
    return tlt_cnt;
}

void TrackerSettings::setTlt_cnt(int value)
{
    if(value < MIN_CNT)
        value = MIN_CNT;
    if(value > MAX_CNT)
        value = MAX_CNT;
    tlt_cnt = value;
}

int TrackerSettings::lpTiltRoll() const
{
    return lptiltroll;
}

void TrackerSettings::setLPTiltRoll(int value)
{
    lptiltroll = value;
}

int TrackerSettings::lpPan() const
{
    return lppan;
}

void TrackerSettings::setLPPan(int value)
{
    lppan = value;
}

int TrackerSettings::gyroWeightTiltRoll() const
{
    return gyroweighttiltroll;
}

void TrackerSettings::setGyroWeightTiltRoll(int value)
{
    gyroweighttiltroll = value;
}

int TrackerSettings::gyroWeightPan() const
{
    return gyroweightpan;
}

void TrackerSettings::setGyroWeightPan(int value)
{
    gyroweightpan = value;
}

char TrackerSettings::servoReverse() const
{
    return servoreverse;
}

void TrackerSettings::setServoreverse(char value)
{
    servoreverse = value;
}

void TrackerSettings::setRollReversed(bool value)
{
    if(value)
        servoreverse |= HT_ROLL_REVERSE_BIT;
    else
        servoreverse &= HT_ROLL_REVERSE_BIT ^ 0xFF;
}

void TrackerSettings::setPanReversed(bool value)
{
    if(value)
        servoreverse |= HT_PAN_REVERSE_BIT;
    else
        servoreverse &= HT_PAN_REVERSE_BIT ^ 0xFF;
}

void TrackerSettings::setTiltReversed(bool value)
{
    if(value)
        servoreverse |= HT_TILT_REVERSE_BIT;
    else
        servoreverse &= HT_TILT_REVERSE_BIT ^ 0xFF;
}

char TrackerSettings::panCh() const
{
    return panch;
}

void TrackerSettings::setPanCh(char value)
{
    if(value > 0 && value < 17)
        panch = value;
}

char TrackerSettings::tiltCh() const
{
    return tiltch;
}

void TrackerSettings::setTiltCh(char value)
{
    if(value > 0 && value < 17)
        tiltch = value;
}

char TrackerSettings::rollCh() const
{
    return rollch;
}

void TrackerSettings::setRollCh(char value)
{
    if(value > 0 && value < 17)
        rollch = value;
}
