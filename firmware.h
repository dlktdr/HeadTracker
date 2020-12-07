#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <QWidget>
#include <QNetworkRequest>
#include <QNetworkReply>

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
    QNetworkReply* reply;

private slots:
    void loadOnlineFirmware();
    void download();
    void downloadFinished(QNetworkReply* data);
    void downloadProgress(qint64 recieved, qint64 total);

};

#endif // FIRMWARE_H
