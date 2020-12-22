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
#include "calibrate.h"

const int MAX_LOG_LENGTH=6000;

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
    QString logd;
    Firmware *firmwareUploader;
    Calibrate *calibratorDialog;

    int xtime;
    bool graphing;

    void parseSerialData();
    void sendSerialData(QByteArray data);
    void sendSerialJSON(QString command, QVariantMap map=QVariantMap());
    void parseInComingJSON(const QVariantMap &map);

    void addToLog(QString log);    

private slots:
    void findSerialPorts();
    void serialConnect();
    void serialDisconnect();
    void updateFromUI();
    void updateToUI();
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
};
#endif // MAINWINDOW_H
