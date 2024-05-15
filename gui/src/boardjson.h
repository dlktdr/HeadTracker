#ifndef BOARDNANO33BLE_H
#define BOARDNANO33BLE_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QQueue>
#include <QTimer>

#include "trackersettings.h"
#include "calibrateble.h"

class BoardJson : public QObject
{
    Q_OBJECT
public:
    BoardJson(TrackerSettings *ts);
    ~BoardJson();

    // Incoming and outgoing data from the board
    void dataIn(QByteArray &recdat);
    QByteArray dataout();

    bool isBoardSavedToRAM() {return savedToRAM;}
    bool isBoardSavedToNVM() {return savedToNVM;}

    QString boardName() {return _boardName;}
    void disconnected();
    void resetCenter();
    void saveToRAM();
    void saveToNVM();
    void reboot();
    void erase();
    void requestHardware();
    void requestFeatures();
    void requestParameters();
    void startCalibration();
    void startData();
    void stopData();
    QStringList getFeatures() {return _features;}
    QMap<QString, QVariant> getPins() {return _pins;}
    static uint16_t escapeCRC(uint16_t crc);
    QByteArray unescapeLog(QByteArray data);

private:
    static const int IMHERETIME=8000; // milliseconds before sending another I'm Here Message to keep communication open
    static const int MAX_TX_FAULTS=8; // Number of times to try re-sending data
    static const int TX_FAULT_PAUSE=750; // milliseconds wait before trying another send
    static const int ACKNAK_TIMEOUT=500; // milliseconds without an ack/nak is a fault

    bool calmsgshowed;
    bool savedToNVM;
    bool savedToRAM;
    bool paramTXErrorSent;
    bool paramRXErrorSent;
    int jsonwaitingack;
    int rxparamfaults;
    bool featuresTXErrorSent;
    bool featuresRXErrorSent;
    int rxfeaturesfaults;
    QStringList _features;
    QMap<QString, QVariant> _pins;

    QByteArray serialDataOut;
    QByteArray lastjson;
    QQueue<QByteArray> jsonqueue;
    QTimer imheretimout;
    QTimer updatesettingstmr;
    QTimer rxParamsTimer;
    QTimer rxFeaturesTimer;
    QTimer reqDataItemsChanged;
    QMap<QString, bool> cursendingdataitems;
    TrackerSettings *trkset;
    CalibrateBLE *bleCalibratorDialog;
    QString _boardName;

    void sendSerialJSON(QString command, QVariantMap map=QVariantMap());
    void parseIncomingJSON(const QVariantMap &map);

    void nakError();

    template<class T>
    class ArrayType {
        ArrayType() {}
    public:
        static const T *getData(const QByteArray &ba, int &len) {
            T* array = (T*)ba.constData();
            len = (int)(ba.size() / sizeof(T));
            return array;
        }
    };

private slots:
    void ihTimeout();
    void rxParamsTimeout();
    void rxFeaturesTimeout();
    void changeDataItems();
    void reqDataItemChanged();
    void calibrationCancel();
    void calibrationComplete();

signals:
    void paramSendStart();
    void paramSendComplete();
    void paramSendFailure(int);
    void paramReceiveStart();
    void paramReceiveComplete();
    void paramReceiveFailure(int);
    void featuresReceiveStart();
    void featuresReceiveComplete();
    void featuresReceiveFailure(int);
    void calibrationSuccess();
    void calibrationFailure();
    void serialTxReady();
    void addToLog(QString log, int ll=0);
    void needsCalibration();
    void boardDiscovered();
    void statusMessage(QString,int timeout=0);
};



#endif // BOARDNANO33BLE_H
