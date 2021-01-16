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
    QProcess *programmer;
    QPlainTextEdit *programmerlog;
    QStringList arguments;
    QString comport;

    void startProgramming(const QString &filename);

    QString programmercommand;

private slots:
    void loadOnlineFirmware();
    void firmwareVersionsReady();
    void firmwareReady();
    void ssLerrors(const QList<QSslError> &errors);
    void replyErrorOccurred(QNetworkReply::NetworkError code);
    void firmwareSelected(QListWidgetItem *lwi);
    void uploadClicked();
    void programmerSTDOUTReady();
    void programmerSTDERRReady();
    void programmerStarted();
    void programmerErrorOccured(QProcess::ProcessError error);
    void programmerFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // FIRMWARE_H
