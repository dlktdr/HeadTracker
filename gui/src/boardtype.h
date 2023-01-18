#ifndef BOARDTYPE_H
#define BOARDTYPE_H

#include <QObject>
#include <QSerialPort>
#include "trackersettings.h"

#define ALWACCESS(r, n, f, p) virtual r n (f p) = 0; \
 r _ ## n (f p) { if(alwaccess) return n ( p ); return r();}

class BoardType : public QObject
{
    Q_OBJECT
public:
    BoardType(QObject *parent=nullptr);

    // Saves the settings storage class
    void setTracker(TrackerSettings *t);

    // Returns the board name
    QString boardName() {return _boardName;}

    // Enables or disables the base classes methods based on alwacces
    virtual void allowAccessChanged(bool access)=0;
    void allowAccess(bool acc) {alwaccess=acc;allowAccessChanged(acc);}
    bool isAccessAllowed() {return alwaccess;}

    // This define creates another method with _ prefix which
    // prevents all methods from working if class is marked
    // disabled (!allowAccess)
    // Only call the _methods
    ALWACCESS(void, dataIn, QByteArray &, recdat)
    ALWACCESS(QByteArray, dataout,,)
    ALWACCESS(bool,isBoardSavedToRAM,,)
    ALWACCESS(bool,isBoardSavedToNVM,,)
    ALWACCESS(void,disconnected,,)
    ALWACCESS(void,resetCenter,,)
    ALWACCESS(void,saveToRAM,,)
    ALWACCESS(void,saveToNVM,,)
    ALWACCESS(void,reboot,,)
    ALWACCESS(void,erase,,)
    ALWACCESS(void,requestHardware,,)
    ALWACCESS(void,requestParameters,,)
    ALWACCESS(void,startCalibration,,)
    ALWACCESS(void,startData,,)
    ALWACCESS(void,stopData,,)

protected:
    TrackerSettings *trkset;
    QString _boardName;

private:
    bool alwaccess;

signals:
    void paramSendStart();
    void paramSendComplete();
    void paramSendFailure(int);
    void paramReceiveStart();
    void paramReceiveComplete();
    void paramReceiveFailure(int);
    void calibrationSuccess();
    void calibrationFailure();
    void serialTxReady();
    void addToLog(QString log, int ll=0);
    void needsCalibration();
    void boardDiscovered(BoardType *);
    void statusMessage(QString,int timeout=0);
};

#endif // BOARDTYPE_H
