#include <QDebug>
#include "trackersettings.h"

TrackerSettings::TrackerSettings(QObject *parent) :
      BaseTrackerSettings(parent)
{
  _setting["axisremap"] = (uint)AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z);
  _setting["axissign"] = (uint)0;
}

void TrackerSettings::setRollReversed(bool value)
{
    if(value)
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() | ROLL_REVERSE_BIT;
    else
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() & (ROLL_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setPanReversed(bool value)
{
    if(value)
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() | PAN_REVERSE_BIT;
    else
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() & (PAN_REVERSE_BIT ^ 0xFFFF);
}

void TrackerSettings::setTiltReversed(bool value)
{
    if(value)
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() | TILT_REVERSE_BIT;
    else
        _setting["servoreverse"] = _setting["servoreverse"].toUInt() & (TILT_REVERSE_BIT ^ 0xFFFF);
}

bool TrackerSettings::isRollReversed()
{
    return (_setting["servoreverse"].toUInt() & ROLL_REVERSE_BIT);
}

bool TrackerSettings::isTiltReversed()
{
    return (_setting["servoreverse"].toUInt() & TILT_REVERSE_BIT);
}

bool TrackerSettings::isPanReversed()
{
    return (_setting["servoreverse"].toUInt() & PAN_REVERSE_BIT);
}

void TrackerSettings::orientation(int &x,int &y,int &z)
{
    x = _setting["rotx"].toInt();
    y = _setting["roty"].toInt();
    z = _setting["rotz"].toInt();
    return;
}

void TrackerSettings::setOrientation(int x,int y,int z)
{
    _setting["rotx"] = x;
    _setting["roty"] = y;
    _setting["rotz"] = z;
}

void TrackerSettings::gyroOffset(float &x, float &y, float &z)
{
    x=_setting["gyrxoff"].toFloat();
    y=_setting["gyryoff"].toFloat();
    z=_setting["gyrzoff"].toFloat();
}

void TrackerSettings::setGyroOffset(float x, float y, float z)
{
    _setting["gyrxoff"]=QString::number(x,'g',3);
    _setting["gyryoff"]=QString::number(y,'g',3);
    _setting["gyrzoff"]=QString::number(z,'g',3);
}

void TrackerSettings::accOffset(float &x, float &y, float &z)
{
    x=_setting["accxoff"].toFloat();
    y=_setting["accyoff"].toFloat();
    z=_setting["acczoff"].toFloat();
}

void TrackerSettings::setAccOffset(float x, float y, float z)
{
    _setting["accxoff"]=QString::number(x,'g',3);
    _setting["accyoff"]=QString::number(y,'g',3);
    _setting["acczoff"]=QString::number(z,'g',3);}

void TrackerSettings::magOffset(float &x, float &y, float &z)
{
    x=_setting["magxoff"].toFloat();
    y=_setting["magyoff"].toFloat();
    z=_setting["magzoff"].toFloat();
}

void TrackerSettings::setMagOffset(float x, float y, float z)
{
    _setting["magxoff"]=QString::number(x,'g',3);
    _setting["magyoff"]=QString::number(y,'g',3);
    _setting["magzoff"]=QString::number(z,'g',3);
}

void TrackerSettings::setAxisRemap(uint value)
{
    _setting["axisremap"] = (uint)(value & 0x3F);
}

void TrackerSettings::setAxisSign(uint value)
{
  _setting["axissign"] = (uint)(value & 0x07);
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

void TrackerSettings::storeSettings(QSettings *settings)
{
    settings->clear();
    QMapIterator<QString, QVariant> i(_setting);
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
        _setting[key] = settings->value(key);
    }
}

QVariantMap TrackerSettings::allData()
{
    return _setting;
}

void TrackerSettings::setAllData(const QVariantMap &data)
{
    QStringList keys = data.keys();
    foreach(QString key,keys) {
        _setting[key] = data.value(key);
        _deviceSettings[key] = data.value(key);
    }
}

QVariantMap TrackerSettings::changedData()
{
    QVariantMap changed;
    QStringList keys = _setting.keys();
    foreach(QString key,keys) {
        // Current Data != Data on Device
        if(_setting[key] != _deviceSettings[key]) {
            changed[key] = _setting[key];
        }
    }
    return changed;
}

void TrackerSettings::setDataMatched()
{
    // Update device data to current data
    _deviceSettings = _setting;
}

QVariant TrackerSettings::liveData(const QString &name)
{
    return _data[name];
}

void TrackerSettings::setLiveData(const QString &name, const QVariant &data)
{
    QVariantMap map;
    map[name] = data;
    setLiveDataMap(map);
}

QVariantMap TrackerSettings::liveDataMap()
{
    return _data;
}

void TrackerSettings::setLiveDataMap(const QVariantMap &datalist, bool reset)
{
    if(reset)
        _data.clear();

    // Update List
    QVariantMap::const_iterator ll;
    for (ll = datalist.begin(); ll != datalist.end(); ++ll)
      _data[ll.key()] = ll.value();

    // Emit if a value has been updated
    bool ge=false,ae=false,me=false,oe=false,ooe=false,ppm=false;
    QMapIterator<QString, QVariant> i(datalist);
    while (i.hasNext()) {
        i.next();
        QString key = i.key();
        if((key == "gyrox" || key == "gyroy" || key == "gyroz") && !ge) {
            emit(rawGyroChanged(_data["gyrox"].toFloat(),
                                _data["gyroy"].toFloat(),
                                _data["gyroz"].toFloat()));
            ge = true;
        }
        if((key == "accx" || key == "accy" || key == "accz") && !ae) {
            emit(rawAccelChanged(_data["accx"].toFloat(),
                                 _data["accy"].toFloat(),
                                 _data["accz"].toFloat()));
            ae = true;
        }
        if((key == "magx" || key == "magy" || key == "magz") && !me) {
            emit(rawMagChanged(_data["magx"].toFloat(),
                                _data["magy"].toFloat(),
                                _data["magz"].toFloat()));
            me = true;
        }
        if((key == "tilt" || key == "roll" || key == "pan") && !oe) {
            emit(rawOrientChanged(_data["tilt"].toFloat(),
                                _data["roll"].toFloat(),
                                _data["pan"].toFloat()));
            oe = true;
        }
        if((key == "tiltoff" || key == "rolloff" || key == "panoff") && !ooe) {
            emit(offOrientChanged(_data["tiltoff"].toFloat(),
                                _data["rolloff"].toFloat(),
                                _data["panoff"].toFloat()));
            ooe = true;
        }
        if((key == "panout" || key == "tiltout" || key == "rollout") && !ppm) {
            emit(ppmOutChanged(_data["tiltout"].toUInt(),
                                _data["rollout"].toUInt(),
                                _data["panout"].toUInt()));
            ppm = true;
        }
        if(key == "btrmt") {
            if(_data["btrmt"] != "")
                emit(bleAddressDiscovered(_data["btrmt"].toString()));
        }
    }

    // Notify data has changed
    emit liveDataChanged();
}

void TrackerSettings::setHardware(QString vers, QString hard, QString git)
{
    _setting["Vers"] = vers;
    _setting["Hard"] = hard;
    _setting["Git"] = git;
}

void TrackerSettings::setSoftIronOffsets(float soo[3][3])
{
    for(int i=0;i < 3; i++){
        for(int j=0;j<3;j++) {
            QString element = QString("so%1%2").arg(i).arg(j);
            _setting[element] = QString::number(soo[i][j],'g',3);
        }
    }
}

// Sets all data items to diabled
void TrackerSettings::clearDataItems()
{
    QMapIterator<QString, bool> i(_dataItems);
    while (i.hasNext()) {
        i.next();
        _dataItems[i.key()] = false;
    }
    setDataItemsMatched();
}

// Add/remove a single item to be sent
void TrackerSettings::setDataItemSend(const QString &itm, const bool &enabled)
{
    if(_dataItems.contains(itm)) {
        if(_dataItems[itm] != enabled) {
            _dataItems[itm] = enabled;
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
    QMapIterator<QString, bool> i(_dataItems);
    while (i.hasNext()) {
        i.next();
        if(_deviceDataItems[i.key()] != i.value()) {
            diffs[i.key()] = i.value();
        }
    }
    return diffs;
}

// Returns the currently sending data items

QMap<QString, bool> TrackerSettings::getDataItems()
{
  return _dataItems;
}
