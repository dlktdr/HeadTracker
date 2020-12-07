
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
    QUrl url = QUrl("https://raw.githubusercontent.com/dlktdr/HeadTracker/master/Firmware/Firmwares.ini");
    QNetworkRequest request(url);

    reply = manager.get(request);
    connect(reply,SIGNAL(finished()),this,SLOT(firmwareVersionsReady()));
}

void Firmware::firmwareVersionsReady()
{
    QFile file("firmwares.ini");
    file.write(reply->readAll());
    file.close();
    QSettings ini("firmwares.ini");
    QStringList titles = ini.childGroups();
    foreach(QString title,titles) {
        QListWidgetItem itm;
        itm.setText(title);
        ui->lstFirmwares->addItem(title);
    }
}
