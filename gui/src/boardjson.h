#ifndef BOARDNANO33BLE_H
#define BOARDNANO33BLE_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QQueue>
#include <QTimer>

#include "boardtype.h"
#include "trackersettings.h"
#include "calibrateble.h"

class BoardJson : public BoardType
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

    void disconnected();
    void resetCenter();
    void saveToRAM();
    void saveToNVM();
    void reboot();
    void erase();
    void requestHardware();
    void requestParameters();
    void startCalibration();
    void startData();
    void stopData();
    void allowAccessChanged(bool acc);
    static uint16_t escapeCRC(uint16_t crc);

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
    QByteArray serialDataOut;
    QByteArray lastjson;
    QQueue<QByteArray> jsonqueue;
    QTimer imheretimout;
    QTimer updatesettingstmr;
    QTimer rxParamsTimer;
    QTimer reqDataItemsChanged;
    QMap<QString, bool> cursendingdataitems;

    CalibrateBLE *bleCalibratorDialog;

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
    void changeDataItems();
    void reqDataItemChanged();
    void calibrationCancel();
    void calibrationComplete();
};



#endif // BOARDNANO33BLE_H
