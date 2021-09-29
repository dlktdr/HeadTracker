#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "manualsend.h"
#include "ui_manualsend.h"
#include "trackersettings.h"
#include "boardnano33ble.h"1
#include "mainwindow.h"
#include "ucrc16lib.h"

ManualSend::ManualSend(QWidget *parent, TrackerSettings *ts) :
    QWidget(parent),
    ui(new Ui::ManualSend),
    trkset(ts)
{
    ui->setupUi(this);    

    connect(ui->cmbParam, &QComboBox::currentTextChanged, this, &ManualSend::commandChange);
    connect(ui->cmdSend, SIGNAL(clicked()), this, SLOT(sendClicked()));
}

ManualSend::~ManualSend()
{
    delete ui;
}

void ManualSend::sendClicked()
{
    if(port->isOpen()) {
        // Don't send any new data until last has been received successfully

        QJsonObject jobj;
        jobj["Cmd"] = ui->cmbCommand->currentText();
        if(ui->cmbCommand->currentText() == "Set")
            jobj[ui->cmbParam->currentText()] = ui->txtVal->text();
        QJsonDocument jdoc(jobj);
        QString json = QJsonDocument(jdoc).toJson(QJsonDocument::Compact);

        // Calculate the CRC Checksum
        uint16_t CRC = BoardNano33BLE::escapeCRC(uCRC16Lib::calculate(json.toUtf8().data(),json.length()));

        QByteArray jsonout = (char)0x02 + json.toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + (char)0x03 + "\r\n";

        port->write(jsonout.data(), jsonout.length());

    }

}

void ManualSend::commandChange()
{
    QString cmd = ui->cmbCommand->currentText();
    ui->cmbParam->clear();
    if(cmd == "Set"){
        ui->cmbParam->addItems(trkset->allData().keys());
        ui->cmbParam->setEditable(true);
    }

}
