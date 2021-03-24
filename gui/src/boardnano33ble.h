#ifndef BOARDNANO33BLE_H
#define BOARDNANO33BLE_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QQueue>
#include <QTimer>

#include "boardtype.h"
#include "ucrc16lib.h"
#include "trackersettings.h"
#include "calibrateble.h"

class BoardNano33BLE : public BoardType
{
    Q_OBJECT
public:
    BoardNano33BLE(TrackerSettings *ts);
    ~BoardNano33BLE();

    // Returns the board name
    QString boardName() {return "NANO33BLE";}

    // Incoming and outgoing data from the board
    void dataIn(QByteArray &recdat);
    QByteArray dataout();

    bool isBoardSavedToRAM() {return savedToRAM;}
    bool isBoardSavedToNVM() {return savedToNVM;}

    void disconnected();
    void resetCenter();
    void saveToRAM();
    void saveToNVM();
    void requestHardware();
    void requestParameters();
    void startCalibration();
    void allowAccessChanged(bool acc);

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
    int jsonfaults;
    int rxparamfaults;
    QByteArray serialDataOut;
    QByteArray lastjson;
    QQueue<QByteArray> jsonqueue;
    QTimer imheretimout;
    QTimer updatesettingstmr;
    QTimer rxParamsTimer;

    CalibrateBLE *bleCalibratorDialog;

    void sendSerialJSON(QString command, QVariantMap map=QVariantMap());
    void parseIncomingJSON(const QVariantMap &map);
    uint16_t escapeCRC(uint16_t crc);
    void nakError();

private slots:

    void ihTimeout();
    void rxParamsTimeout();
};

#endif // BOARDNANO33BLE_H
