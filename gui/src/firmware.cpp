
#include <QNetworkAccessManager>
#include <QSettings>
#include <QTimer>
#include <QScrollBar>
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
    reply = nullptr;

    // AVR Dude Programmer
    avrdude = new QProcess(this);
    avrdudelog = new QPlainTextEdit;
    avrdudelog->setWindowTitle("Firmware Programming Output");
    avrdudecommand = "avrdude.exe";
    connect(avrdude, SIGNAL(readyReadStandardOutput()), this, SLOT(avrDudeSTDOUTReady()));
    connect(avrdude, SIGNAL(readyReadStandardError()), this, SLOT(avrDudeSTDERRReady()));
    connect(avrdude, SIGNAL(started()), this, SLOT(avrDudeStarted()));
    connect(avrdude, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(avrDudeFinished(int, QProcess::ExitStatus)));
    connect(avrdude, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(avrDudeErrorOccured(QProcess::ProcessError)));
}

Firmware::~Firmware()
{
    delete ui;
    if (reply != nullptr)
        delete reply;
    if (firmreply != nullptr)
        delete reply;

    delete avrdude;
    delete avrdudelog;
}

void Firmware::startProgramming(const QString &filename)
{
    // Call the AVR Dude command to program it now.
    QListWidgetItem *itm = ui->lstFirmwares->currentItem();
    if(itm == nullptr)
        return;

    QMap<QString, QVariant>data = itm->data(Qt::UserRole).toMap();

    if(data["Uploader"].isNull()) {

        arguments = data["args"].toStringList();

        arguments.append(QString("-Uflash:w:%1").arg(filename));
        arguments.append(QString("-P%1").arg(comport));
        arguments.append(QString("-Cavrdude.conf"));
        avrdude->start(avrdudecommand, arguments);


    // Add ability to use different uploaders. %1 is the comport, %2 is the filename
    } else {
        avrdudecommand = data["Uploader"].toString();
        arguments = data["args"].toString().arg(comport).arg(filename).split(' ');
        avrdude->start(avrdudecommand, arguments);
    }
}

void Firmware::loadOnlineFirmware()
{
    QUrl url = QUrl(baseurl + "Firmwares.ini");
    QNetworkRequest request(url);
    reply = manager.get(request);
    connect(reply,SIGNAL(finished()),this,SLOT(firmwareVersionsReady()));
    connect(reply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
    connect(reply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(replyErrorOccurred(QNetworkReply::NetworkError)));
}

void Firmware::firmwareVersionsReady()
{
    // Store the remote file localy
    QByteArray ba;
    QFile file("Firmwares.ini");
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    ba = reply->readAll();
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
    reply->deleteLater();
}

void Firmware::firmwareReady()
{
    qDebug() << "Got the Firmware";

    // Store the remote file localy
    QByteArray ba;
    QFile file(localfilename);
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    ba = firmreply->readAll();
    file.write(ba);
    file.flush();
    file.close();

    // Upload it
    startProgramming(localfilename);

    // Delete it
    firmreply->deleteLater();
}

void Firmware::ssLerrors(const QList<QSslError> &errors)
{
    if(errors.length())
        QMessageBox::critical(this,"Error",errors.at(0).errorString());
}

void Firmware::replyErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    QMessageBox::critical(this,"Error",firmreply->errorString());

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
    ui->infoTable->horizontalHeader()->setStretchLastSection(true);
    ui->infoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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
    firmreply = manager.get(request);
    connect(firmreply,SIGNAL(finished()),this,SLOT(firmwareReady()));
    connect(firmreply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
    connect(firmreply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(replyErrorOccurred(QNetworkReply::NetworkError)));

    avrdudelog->clear();
    avrdudelog->show();
    avrdudelog->setMinimumSize(640,480);
    avrdudelog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    avrdudelog->appendPlainText("Downloading " + url.toString() + "\n");
}

void Firmware::avrDudeSTDOUTReady()
{
    QByteArray ba = avrdude->readAllStandardOutput();
    avrdudelog->setPlainText(avrdudelog->toPlainText() + ba);
    avrdudelog->verticalScrollBar()->setValue(avrdudelog->verticalScrollBar()->maximum());
}

void Firmware::avrDudeSTDERRReady()
{
    QByteArray ba = avrdude->readAllStandardError();
    avrdudelog->setPlainText(avrdudelog->toPlainText() + ba);
    avrdudelog->verticalScrollBar()->setValue(avrdudelog->verticalScrollBar()->maximum());
}

void Firmware::avrDudeStarted()
{
    avrdudelog->appendPlainText("Starting " + avrdudecommand + " " + arguments.join(" ") + "\n");
}

void Firmware::avrDudeErrorOccured(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart: {
        QMessageBox::critical(this, "Error", "Unable to open " + avrdudecommand);
        break;
    }
    default: {
        QMessageBox::critical(this, "Error", QString("Process Error %1").arg(error));
            }
    }
}

void Firmware::avrDudeFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    Q_UNUSED(exitCode);

    if(exitCode == 0) {
        // If Successfull autoclose window.
        QTimer::singleShot(1500,avrdudelog,SLOT(close()));
        QMessageBox::about(this, "Programming Success", "Programming Successful!");
    } else {
        QMessageBox::critical(this, "Programming Failure", "Programming Failed, Please check the log for information\nFor Nane33BLE Double tap reset button to enter programming mode.\nClose this window and verify proper com port");
    }
}
