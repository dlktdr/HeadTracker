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

// !!!! Move me to a adjustable settings file !!!!! **********************
const QString baseurl = "https://raw.githubusercontent.com/dlktdr/HeadTracker/master/firmware/bin/";
const QString avrdudecommand = "avrdude.exe";
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
    void setComPort(QString port) {comport = port;}

private:
    Ui::Firmware *ui;
    QNetworkAccessManager manager;
    QNetworkReply* reply;
    QNetworkReply* firmreply;
    QProcess *avrdude;
    QPlainTextEdit *avrdudelog;
    QStringList arguments;
    QString comport;

    void startProgramming(const QString &filename);

private slots:
    void loadOnlineFirmware();
    void firmwareVersionsReady();
    void firmwareReady();
    void ssLerrors(const QList<QSslError> &errors);
    void replyErrorOccurred(QNetworkReply::NetworkError code);
    void firmwareSelected(QListWidgetItem *lwi);
    void uploadClicked();
    void avrDudeSTDOUTReady();
    void avrDudeSTDERRReady();
    void avrDudeStarted();
    void avrDudeErrorOccured(QProcess::ProcessError error);
    void avrDudeFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // FIRMWARE_H
