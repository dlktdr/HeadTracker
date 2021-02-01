
#include <QNetworkAccessManager>
#include <QSettings>
#include <QTimer>
#include <QScrollBar>
#include "mainwindow.h"
#include "firmware.h"
#include "ui_firmware.h"

Firmware::Firmware(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Firmware)
{
    ui->setupUi(this);
    connect(ui->cmdFetch,SIGNAL(clicked()),this,SLOT(loadOnlineFirmware()));
    connect(ui->lstFirmwares,SIGNAL(itemClicked(QListWidgetItem *)),this,SLOT(firmwareSelected(QListWidgetItem *)));
    connect(ui->cmdClose,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->cmdUploadOnline,SIGNAL(clicked()),this,SLOT(uploadClicked()));
    connect(ui->cmdStopUpload,SIGNAL(clicked()),this,SLOT(cmdKillClick()));
    connect(ui->cmdRefreshPorts,SIGNAL(clicked()),this,SLOT(findSerialPorts()));
    hexreply = nullptr;
    firmreply = nullptr;

    // Programmer
    programmer = new QProcess(this);
    programmerlog = ui->outputLog;
    programmercommand = "programmer.exe";
    connect(programmer, SIGNAL(readyReadStandardOutput()), this, SLOT(programmerSTDOUTReady()));
    connect(programmer, SIGNAL(readyReadStandardError()), this, SLOT(programmerSTDERRReady()));
    connect(programmer, SIGNAL(started()), this, SLOT(programmerStarted()));
    connect(programmer, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(programmerFinished(int, QProcess::ExitStatus)));
    connect(programmer, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(programmerErrorOccured(QProcess::ProcessError)));
    ui->cmdStopUpload->setEnabled(false);
    findSerialPorts();
}

Firmware::~Firmware()
{
    delete ui;
    if (hexreply != nullptr)
        delete hexreply;
    if (firmreply != nullptr)
        delete firmreply;

    delete programmer;
    delete programmerlog;
}

// Finds available serial ports
void Firmware::findSerialPorts()
{
    ui->cmbPort->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port,ports) {
        ui->cmbPort->addItem(port.portName(),port.serialNumber());
    }
}

void Firmware::startProgramming(const QString &filename)
{
    // Call the AVR Dude command to program it now.
    QListWidgetItem *itm = ui->lstFirmwares->currentItem();
    if(itm == nullptr)
        return;

    QMap<QString, QVariant>data = itm->data(Qt::UserRole).toMap();

    programmercommand = data["Uploader"].toString();
    QString args = data["args"].toStringList().join(' ');
    arguments = args.arg(ui->cmbPort->currentText()).arg(filename).split(' ');
    qDebug() << "STARTING:" <<programmercommand << arguments;
    programmer->start(programmercommand, arguments);
}

void Firmware::loadOnlineFirmware()
{
    QUrl url = QUrl(baseurl + QString("firmware%1.ini").arg(fwversion));
    qDebug() << url;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute,false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,false);
    firmreply = manager.get(request);
    connect(firmreply,SIGNAL(finished()),this,SLOT(firmwareVersionsReady()));
    connect(firmreply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
    connect(firmreply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(firmReplyErrorOccurred(QNetworkReply::NetworkError)));
}

void Firmware::firmwareVersionsReady()
{
    // Store the remote file localy
    QByteArray ba;
    QFile file("Firmwares.ini");
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    ba = firmreply->readAll();
    file.write(ba);
    file.flush();
    file.close();

    // Use QSettings to parse the Ini File
    QSettings ini("Firmwares.ini",QSettings::IniFormat);
    QStringList titles = ini.childGroups();
    ui->lstFirmwares->clear();
    foreach(QString title,titles) {
        ini.beginGroup(title);
        // Create a new Item with title
        QListWidgetItem *itm = new QListWidgetItem(title);

        // Extract all the variant keys, make a new map and add them to mapData
        QMap<QString,QVariant> mapData;
        foreach(QString childkey, ini.childKeys()) {
            if(childkey.toLower() == "filename")
                childkey = "filename";
            mapData[childkey] = ini.value(childkey);
        }
        itm->setData(Qt::UserRole,mapData);
        ui->lstFirmwares->addItem(itm);
        ini.endGroup();
    }

    // Delete it
    firmreply->deleteLater();
}

void Firmware::firmwareReady()
{
    qDebug() << "Got the Firmware";

    // Store the remote file localy
    QByteArray ba;
    QFile file(localfilename);
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    ba = hexreply->readAll();
    file.write(ba);
    file.flush();
    file.close();

    // Upload it
    startProgramming(localfilename);

    // Delete it
    hexreply->deleteLater();
}

void Firmware::ssLerrors(const QList<QSslError> &errors)
{
    if(errors.size() > 0)
        QMessageBox::critical(this,"Error",errors.at(0).errorString());
}

void Firmware::firmReplyErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(firmreply != nullptr)
        QMessageBox::critical(this,"Fetching Firmwares Error",firmreply->errorString());
}


void Firmware::hexReplyErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(hexreply != nullptr)
        QMessageBox::critical(this,"Fetching Binary Error",hexreply->errorString());
}

/*  Updates the UI to extract the information from the selected firmware
 */
void Firmware::firmwareSelected(QListWidgetItem *lwi)
{
    int row=0;
    // Get the data
    QMap<QString, QVariant>data = lwi->data(Qt::UserRole).toMap();

    // Setup Table
    ui->infoTable->clear();
    ui->infoTable->setColumnCount(2);
    ui->infoTable->setRowCount(row = data.count());

    // Loop through data
    QMapIterator<QString, QVariant> i(data);
    while (i.hasNext()) {
        row--;
        i.next();
        QTableWidgetItem *text = new QTableWidgetItem(i.key());
        QTableWidgetItem *value;
        if(i.key().toLower().contains("addr"))
            value = new QTableWidgetItem("0x" + QString::number(i.value().toInt(),16));
        else
            value = new QTableWidgetItem(i.value().toString());

        if(i.key().toLower().contains("args"))
            value = new QTableWidgetItem(i.value().toStringList().join(" "));

        ui->infoTable->setItem(row,0,text);
        ui->infoTable->setItem(row,1,value);
    }
    ui->infoTable->resizeColumnsToContents();
}

void Firmware::uploadClicked()
{
    // Get the online filename
    QListWidgetItem *itm = ui->lstFirmwares->currentItem();
    if(itm == nullptr)
        return;
    QMap<QString, QVariant>data = itm->data(Qt::UserRole).toMap();

    // First Fetch the Firmware File from GitHub
    QUrl url = baseurl + QUrl::toPercentEncoding(data["filename"].toString());

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute,false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,false);
    hexreply = manager.get(request);
    connect(hexreply,SIGNAL(finished()),this,SLOT(firmwareReady()));
    connect(hexreply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
    connect(hexreply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(replyErrorOccurred(QNetworkReply::NetworkError)));

    programmerlog->clear();
    programmerlog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    programmerlog->appendPlainText("Downloading " + url.toString() + "\n");
}

void Firmware::cmdKillClick()
{
    if(programmer->isOpen())
        programmer->kill();
    programmerlog->setPlainText(programmerlog->toPlainText() + "\nMANUALLY KILLED PROCESS");
}

void Firmware::programmerSTDOUTReady()
{
    QByteArray ba = programmer->readAllStandardOutput();
    programmerlog->setPlainText(programmerlog->toPlainText() + ba);
    programmerlog->verticalScrollBar()->setValue(programmerlog->verticalScrollBar()->maximum());
}

void Firmware::programmerSTDERRReady()
{
    QByteArray ba = programmer->readAllStandardError();
    programmerlog->setPlainText(programmerlog->toPlainText() + ba);
    programmerlog->verticalScrollBar()->setValue(programmerlog->verticalScrollBar()->maximum());
}

void Firmware::programmerStarted()
{
    programmerlog->appendPlainText("Starting " + programmercommand + " " + arguments.join(" ") + "\n");
    ui->cmdStopUpload->setEnabled(true);
    ui->cmdUploadOnline->setEnabled(false);
}

void Firmware::programmerErrorOccured(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart: {
        QMessageBox::critical(this, "Error", "Unable to open " + programmercommand);
        break;
    }
    default: {
        QMessageBox::critical(this, "Error", QString("Process Error %1").arg(error));
            }
    }
    ui->cmdStopUpload->setEnabled(false);
    ui->cmdUploadOnline->setEnabled(true);
}

void Firmware::programmerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    Q_UNUSED(exitCode);

    if(exitCode == 0) {
        QMessageBox::about(this, "Programming Success", "Programming Successful!");
    } else {
        QMessageBox::critical(this, "Programming Failure", "Programming Failed, Please check the log for information\n\nFor the Nane33BLE Double tap reset button to enter programming mode.\nRefresh COM ports and verify you have the correct one selected");
    }
    ui->cmdStopUpload->setEnabled(false);
    ui->cmdUploadOnline->setEnabled(true);
}

void Firmware::showEvent(QShowEvent *event)
{
    loadOnlineFirmware();
    event->accept();
}
