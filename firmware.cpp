
#include <QNetworkAccessManager>
#include "firmware.h"
#include "ui_firmware.h"

Firmware::Firmware(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Firmware)
{
    ui->setupUi(this);
    connect(ui->cmdFetch,SIGNAL(clicked()),this,SLOT(loadOnlineFirmware()));
}

Firmware::~Firmware()
{
    delete ui;
}

void Firmware::loadOnlineFirmware()
{
    QNetworkAccessManager access;



}
