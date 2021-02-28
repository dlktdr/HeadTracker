#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QScrollBar>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QQueue>
#include "trackersettings.h"
#include "firmware.h"
#include "calibratebno.h"
#include "calibrateble.h"
#include "diagnosticdisplay.h"

const int MAX_LOG_LENGTH=6000; // How many bytes to keep of log data in the gui
const QString version="0.8"; // Current Version Number
const QString fwversion="04"; // Which suffix on firmware file to use from GITHUB
const QStringList firmwares={"BNO055","NANO33BLE"}; // Allowable hardware types

const int IMHERETIME=8000; // milliseconds before sending another I'm Here Message to keep communication open
const int MAX_TX_FAULTS=8; // Number of times to try re-sending data
const int TX_FAULT_PAUSE=750; // milliseconds wait before trying another send
const int ACKNAK_TIMEOUT=500; // milliseconds without an ack/nak is a fault

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QSerialPort *serialcon;
    TrackerSettings trkset;
    QByteArray serialData;
    QTimer rxledtimer;
    QTimer txledtimer;
    QTimer updatesettingstmr;
    QTimer acknowledge;
    QTimer comtimeout;
    QString logd;
    Firmware *firmwareUploader;
    CalibrateBNO *bnoCalibratorDialog;
    CalibrateBLE *bleCalibratorDialog;
    DiagnosticDisplay *diagnostic;
    QMessageBox msgbox;
    bool savedToNVM;
    bool sentToHT;
    bool fwdiscovered;
    bool calmsgshowed;
    bool settingstoHT;

    int jsonfaults;
    QByteArray lastjson;
    QQueue<QByteArray> jsonqueue;

    bool graphing;
    bool rawmode;

    void parseSerialData();
    void sendSerialData(QByteArray data);

    // HT Format
    void parseIncomingHT(QString cmd);
    // JSON format
    void sendSerialJSON(QString command, QVariantMap map=QVariantMap());
    void parseIncomingJSON(const QVariantMap &map);
    void fwDiscovered(QString vers, QString hard);
    void addToLog(QString log);


protected:
    void keyPressEvent(QKeyEvent *event);


private slots:
    void findSerialPorts();
    void serialConnect();
    void serialDisconnect();
    void serialError(QSerialPort::SerialPortError);
    void connectTimeout();
    void comTimeout();
    void updateFromUI();
    void updateToUI();
    void offOrientChanged(float,float,float);
    void ppmOutChanged(int,int,int);
    void serialReadReady();
    void manualSend();
    void startGraph();
    void stopGraph();
    void storeSettings(); // Save to eeprom
    void updateSettings(); // Update to chip
    void resetCenter();
    void setDataMode(bool);
    void requestTimer();
    void rxledtimeout();
    void txledtimeout();
    void saveSettings();
    void loadSettings();
    void uploadFirmwareClick();
    void startCalibration();
    void ihTimeout();
    void saveToNVM();
    void showDiagsClicked();

};
#endif // MAINWINDOW_H
