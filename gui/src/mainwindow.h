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
#include <QTextEdit>
#include "trackersettings.h"
#include "firmwarewizard.h"
#include "channelviewer.h"
#include "diagnosticdisplay.h"
#include "boardjson.h"
#include "imageviewer/imageviewer.h"

#ifndef GIT_CURRENT_SHA
#define GIT_CURRENT_SHA "SHA Unknown"
#endif

#ifndef GIT_VERSION_TAG
#define GIT_VERSION_TAG "?.??"
#endif

#define STRINGIZER(arg) #arg
#define STR_VALUE(arg) STRINGIZER(arg)
#define GIT_VERSION_TAG_STRING STR_VALUE(GIT_VERSION_TAG)
#define GIT_CURRENT_SHA_STRING STR_VALUE(GIT_CURRENT_SHA)

constexpr int MAX_LOG_LENGTH=6000; // How many bytes to keep of log data in the gui
const QString version=GIT_VERSION_TAG_STRING; // Current Version Number
const QStringList firmwares={"BNO055","NANO33BLE"}; // Allowable hardware types
const QUrl helpurl("https://headtracker.gitbook.io/head-tracker-v2.2/settings/gui-settings");
const QUrl discordurl("https://discord.gg/ux5hEaNSPQ");
const QUrl githuburl("https://github.com/dlktdr/HeadTracker");
const QUrl donateurl("https://www.paypal.com/donate?hosted_button_id=NMU3B9Z82JB3A");

constexpr int IMHERETIME=8000; // milliseconds before sending another I'm Here Message to keep communication open
constexpr int MAX_TX_FAULTS=8; // Number of times to try re-sending data
constexpr int TX_FAULT_PAUSE=750; // milliseconds wait before trying another send
constexpr int ACKNAK_TIMEOUT=500; // milliseconds without an ack/nak is a fault
constexpr int RECONNECT_AFT_REBT=3000; // (ms) time after a reboot to try to reconnect.
constexpr int WAIT_FOR_BOARD_TO_BOOT=1000;
constexpr int WAIT_BETWEEN_BOARD_CONNECTIONS=1500;

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
    QTimer requestTimer;
    QTimer connectTimer;
    QTimer saveToRAMTimer;
    QTimer requestParamsTimer;

    bool waitingOnParameters;
    bool waitingOnFeatures;
    bool boardDiscover;
    bool boardDiscoveryStarted;

    QMap<QString, bool> cursendingdataitems;
    bool setOrientationStart;
    bool setForwardStart;

    QString logd;    
    FirmwareWizard *firmwareWizard;
    DiagnosticDisplay *diagnostic;
    ChannelViewer *channelviewer;
    QMessageBox *msgbox;
    QTextEdit *serialDebug;
    ImageViewer *imageViewer;
    QStringList bleaddrs;

    QQueue<QByteArray> serialDataOut;
    volatile bool sending;

    BoardJson *jsonht;
    bool channelViewerOpen=false;

    void parseSerialData();
    void sendSerialData(QByteArray data);
    void slowSerialSend();
    bool checkSaved();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void addToLog(QString log, int ll=0);
    void findSerialPorts();
    void connectDisconnectClicked();
    void serialConnect();
    void serialDisconnect();
    void serialError(QSerialPort::SerialPortError);
    void connectTimeout();
    void requestTimeout();
    void saveToRAMTimeout();
    void requestParamsTimeout();
    void eraseFlash();
    void updateFromUI();
    void updateToUI();
    void offOrientChanged(float,float,float);
    void ppmOutChanged(int,int,int);
    void bleAddressDiscovered(QString str);
    void liveDataChanged();
    void serialReadReady();
    void storeToNVM();
    void storeToRAM();
    void resetCenter();
    void rxledtimeout();
    void txledtimeout();
    void saveSettings();
    void loadSettings();
    void uploadFirmwareWizard();
    void startCalibration();
    void startData();
    void showDiagsClicked();
    void showSerialDiagClicked();
    void showChannelViewerClicked();
    void BLE33tabChanged();
    void reboot();
    void openHelp();
    void openDiscord();
    void openDonate();
    void openGitHub();
    void showPinView();
    void setOrientationClicked();
    void setForwardClicked();
    void calAccelChanged(float x, float y, float z);
    void quaternionChanged(QQuaternion q);

    // Board Connections
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
    void needsCalibration();
    void boardDiscovered();
    void statusMessage(QString,int timeout=0);
};
#endif // MAINWINDOW_H
