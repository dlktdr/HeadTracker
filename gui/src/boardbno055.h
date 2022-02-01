#ifndef BOARDBNO055_H
#define BOARDBNO055_H

#include <QObject>
#include <QTimer>

#include "boardtype.h"
#include "ucrc16lib.h"
#include "calibratebno.h"

class BoardBNO055 : public BoardType
{
    Q_OBJECT
public:
    BoardBNO055(TrackerSettings *ts);
    ~BoardBNO055();

    // Returns the board name
    QString boardName() {return "BNO055";}

    // Incoming and outgoing data from the board
    void dataIn(QByteArray &recdat);
    QByteArray dataout();

    bool isBoardSavedToRAM() {return savedToRAM;}
    bool isBoardSavedToNVM() {return savedToNVM;}

    void disconnected();
    void resetCenter();
    void saveToRAM();
    void saveToNVM();
    void reboot() {}
    void erase() {}
    void requestHardware();
    void requestParameters();
    void startCalibration();
    void startData();
    void stopData() {}
    void allowAccessChanged(bool acc);

private:
    bool savedToNVM;
    bool savedToRAM;
    bool graphing;
    bool rawmode;
    QString vers;
    QString hard;

    QByteArray serialDataOut;

    CalibrateBNO *bnoCalibratorDialog;

    void parseIncomingHT(QString cmd);
    uint16_t escapeCRC(uint16_t crc);
    void startGraph();
    void stopGraph();
    void setDataMode(bool);
private slots:
    void comTimeout();

};

#endif // BOARDBNO055_H
