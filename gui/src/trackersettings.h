#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <QObject>
#include <QSettings>

#include "basetrackersettings.h"

// Axis Mapping
#define AXIS_X 0x00
#define AXIS_Y 0x01
#define AXIS_Z 0x02
#define AXES_MAP(XX,YY,ZZ) (ZZ<<4|YY<<2|XX)
#define X_REV 0x04
#define Y_REV 0x02
#define Z_REV 0x01

class TrackerSettings : public BaseTrackerSettings
{    
    Q_OBJECT
public:
    enum {BTDISABLE,BTPARAHEAD,BTPARARMT};

    TrackerSettings(QObject *parent=nullptr);

    void setRollReversed(bool value);
    void setPanReversed(bool Value);
    void setTiltReversed(bool Value);
    bool isRollReversed();
    bool isTiltReversed();
    bool isPanReversed();

    void orientation(int &x,int &y,int &z);
    void setOrientation(int x,int y,int z);

    void gyroOffset(float &x, float &y, float &z);
    void setGyroOffset(float x,float y, float z);

    void accOffset(float &x, float &y, float &z);
    void setAccOffset(float x,float y, float z);

    void magOffset(float &x, float &y, float &z);
    void setMagOffset(float x,float y, float z);  

    // OLD BNO Settings..
    int count() const {return 22;} //BNO, how many values should there be
    uint axisRemap() const {return _setting["axisremap"].toUInt();}
    void setAxisRemap(uint value);        
    uint axisSign() const {return _setting["axissign"].toUInt();}
    void setAxisSign(uint value);
    int gyroWeightPan() const;
    void setGyroWeightPan(int value);
    int gyroWeightTiltRoll() const;
    void setGyroWeightTiltRoll(int value);

    void storeSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

    QVariantMap allData();
    void setAllData(const QVariantMap &data);

    QVariantMap changedData();
    void setDataMatched();

    QVariant liveData(const QString &name);
    void setLiveData(const QString &name, const QVariant &live);
    void clearLiveData() {_data.clear();}

    QVariantMap liveDataMap();
    void setLiveDataMap(const QVariantMap &livelist, bool clear=false);

    QString hardware() {return _setting["Hard"].toString();}
    QString fwVersion() {return _setting["Vers"].toString();}
    void setHardware(QString vers,QString hard, QString git="");

    void setSoftIronOffsets(float soo[3][3]);

    void clear() {_data.clear(); _setting.clear();}

    // Realtime data requested from the board
    // Keeps track of them in _data2send list
    void clearDataItems();
    void setDataItemSend(const QString &itm, const bool &enabled);
    void setDataItemSend(QMap<QString,bool> items);
    QMap<QString, bool> getDataItemsDiff();
    void setDataItemsMatched() {_deviceDataItems = _dataItems;}
    // Gets all currently sending data items
    QMap<QString, bool> getDataItems();    

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
    QStringList bleAddresses;
};

#endif // TRACKERSETTINGS_H
