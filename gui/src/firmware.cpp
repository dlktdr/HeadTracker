
#include <QNetworkAccessManager>
#include <QSettings>
#include "firmware.h"
#include "ui_firmware.h"

Firmware::Firmware(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Firmware)
{
    ui->setupUi(this);
    connect(ui->cmdFetch,SIGNAL(clicked()),this,SLOT(loadOnlineFirmware()));
    reply = nullptr;
}

Firmware::~Firmware()
{
    delete ui;
    if (reply != nullptr)
        delete reply;
}

void Firmware::loadOnlineFirmware()
{
    QUrl url = QUrl(baseurl + "firmwares.ini");
    QNetworkRequest request(url);
    reply = manager.get(request);
    connect(reply,SIGNAL(finished()),this,SLOT(firmwareVersionsReady()));
    connect(reply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
}

void Firmware::firmwareVersionsReady()
{
    QByteArray ba;
    QFile file("firmwares.ini");
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    ba = reply->readAll();
    qDebug() << ba;
    file.write(ba);
    file.flush();
    file.close();
    QSettings ini("firmwares.ini",QSettings::IniFormat);
    ini.clear();

    ini.beginGroup("Arduino Nano V0.1 - BNOAdr(0x29), Btn(D11) PWM(D9)");
    ini.setValue("FileName","nano 0.1a.hex");
    ini.setValue("Version", 0.1);
    ini.setValue("Button","D11");
    ini.setValue("BNO055Addr","41");
    ini.endGroup();

    qDebug() << ini.allKeys();
    QStringList titles = ini.childGroups();
    foreach(QString title,titles) {
        QListWidgetItem itm;
        itm.setText(title);
        ui->lstFirmwares->addItem(title);
    }
}

void Firmware::ssLerrors(const QList<QSslError> &errors)
{
    qDebug() << errors;
}
