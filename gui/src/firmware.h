#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <QWidget>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QListWidgetItem>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QPlainTextEdit>
#include <QDomDocument>

// !!!! Move me to a adjustable settings file !!!!! **********************
const QString baseurl = "https://raw.githubusercontent.com/dlktdr/HeadTracker/master/firmware/bin/";

const QString localfilename = "online.hex";


namespace Ui {
class Firmware;
}

class Firmware : public QWidget
{
    Q_OBJECT

public:
    explicit Firmware(QWidget *parent = nullptr);
    ~Firmware();    

private:
    Ui::Firmware *ui;
    QNetworkAccessManager manager;
    QNetworkReply* hexreply;
    QNetworkReply* firmreply;
    QProcess *programmer;
    QPlainTextEdit *programmerlog;
    QStringList arguments;
    QString comport;
    QDomDocument xmlfirmwares;

    void startProgramming(const QString &filename);

    QString programmercommand;

private slots:
    void findSerialPorts();
    void loadLocalFirmware();
    void loadOnlineFirmware();
    void firmwareVersionsReady();
    void firmwareReady();
    void ssLerrors(const QList<QSslError> &errors);
    void firmReplyErrorOccurred(QNetworkReply::NetworkError code);
    void hexReplyErrorOccurred(QNetworkReply::NetworkError code);
    void firmwareSelected(QListWidgetItem *lwi);
    void uploadClicked();
    void cmdKillClick();
    void programmerSTDOUTReady();
    void programmerSTDERRReady();
    void programmerStarted();
    void programmerErrorOccured(QProcess::ProcessError error);
    void programmerFinished(int exitCode, QProcess::ExitStatus exitStatus);
protected:
    void showEvent(QShowEvent *event);
};

#endif // FIRMWARE_H
