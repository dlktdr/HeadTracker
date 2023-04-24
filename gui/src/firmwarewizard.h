#ifndef FirmwareWizard_H
#define FirmwareWizard_H

#include <QWidget>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QListWidgetItem>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QPlainTextEdit>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QSettings>

#if defined(LINUX)
    constexpr char bossac_programmer[] = "bossac_lin";
#elif defined(WINDOWS)
    constexpr char bossac_programmer[] = "bossac.exe";
#elif defined(MACOS)
    constexpr char bossac_programmer[] = "bossac_mac";
#endif

const QString localfirmlist = "firmware.ini";
const QString localfirmware = "online.fmw";

const QByteArray BLE33HEADER_BIN_MBED("\x00\x00\x04\x20",4);
const QByteArray BLE33HEADER_BIN_ZEPHER("\x00\x20",2);
const QByteArray BLE33HEADER_HEX(":020000021000EC",15);
const QByteArray NANOHEADER_HEX(":100000000C94",13);

constexpr int REFRESH_DELAY = 10; // 10 * 200ms = 2sec

namespace Ui {
class firmwarewizard;
}

class FirmwareWizard : public QWidget
{
    Q_OBJECT

public:
    explicit FirmwareWizard(QWidget *parent = nullptr);
    ~FirmwareWizard();

private:
    Ui::firmwarewizard *ui;

    int waitingprogram;

    typedef enum {
        PRG_IDLE,
        PRG_REQUEST_FIRMWARE,
        PRG_GOT_FIRMWARE,
        PRG_WAIT4PORT,
        PRG_SETBOOTLOAD,
        PRG_WAIT4NEWPORT,
        PRG_PROGRAM_START,
        PRG_DOWNLOADING,
        PRG_PROGRAM_END,
        PRG_CONNECT,
        PRG_UPLOADSETTINGS,
        PRG_DISCONNECT,
        PRG_COMPLETE
    } prgstate;
    prgstate programmerState;

    typedef enum {
        BRD_UNKNOWN,
        BRD_NANO33BLE,
        BRD_ARDUINONANO
    } boardtype;
    boardtype boardType;

    QNetworkAccessManager manager;
    QNetworkReply* hexreply;
    QNetworkReply* firmreply;

    QString firmwarefile;

    QPlainTextEdit *programmerlog;
    QProcess *programmer;
    QStringList arguments;
    QString programmercommand;

    QStringList lastports;
    QTimer discoverPortTmr;
    QTimer lastPortTmr;
    QString lastFoundPort;

    int refreshDelay=0;

    void startPortDiscovery(const QString &filename);
    void startProgramming();
    void parseFirmwareFile(QString ff);
    void setState(prgstate state);
    void discoverPorts(bool init=false);
    void initalize();
    void addToLog(QString);

private slots:
    //void findSerialPorts();
    void loadLocalFirmware();
    void loadOnlineFirmware();
    void firmwareVersionsReady();
    void firmwareReady();
    void ssLerrors(const QList<QSslError> &errors);
    void firmReplyErrorOccurred(QNetworkReply::NetworkError code);
    void hexReplyErrorOccurred(QNetworkReply::NetworkError code);
    void firmwareSelected(QListWidgetItem *lwi);
    void programClicked();
    void programmerSTDOUTReady();
    void programmerSTDERRReady();
    void programmerStarted();
    void programmerErrorOccured(QProcess::ProcessError error);
    void programmerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void setBootloader();
    void comPortConnect(QString comport);
    void cmbSourceChanged(int);
    void discoverTimeout();
    void portDiscoverTimeout();
    void closeClicked();
    void backClicked();
    void chkShowLogClicked(int);

signals:
    void comPortConnected(QString);
    void programmingComplete();

protected:
    void showEvent(QShowEvent *event);
};

#endif // FirmwareWizard_H
