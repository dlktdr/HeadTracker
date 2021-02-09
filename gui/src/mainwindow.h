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
#include "trackersettings.h"
#include "firmware.h"
#include "calibratebno.h"
#include "calibrateble.h"
#include "diagnosticdisplay.h"

const int MAX_LOG_LENGTH=6000;
const QString version="0.51";
const QString fwversion="04"; // Which Firmware file to Use
const QStringList firmwares={"BNO055","NANO33BLE","REMOTEBLE"};

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

    int xtime;
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
    void ackTimeout();
    void saveToNVM();

};
#endif // MAINWINDOW_H
