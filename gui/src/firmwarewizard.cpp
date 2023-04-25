#include <QNetworkAccessManager>
#include <QSettings>
#include <QTimer>
#include <QScrollBar>
#include <QFileDialog>
#include <QMap>
#include <QComboBox>

#include "firmwarewizard.h"
#include "ui_firmwarewizard.h"

FirmwareWizard::FirmwareWizard(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::firmwarewizard)
{
    ui->setupUi(this);

    // Programmer
    programmer = new QProcess(this);

    // Programmer Log
    programmerlog = new QPlainTextEdit;
    programmerlog->setWindowTitle("Programmer Log");
    connect(ui->lstFirmwares,SIGNAL(itemClicked(QListWidgetItem *)),this,SLOT(firmwareSelected(QListWidgetItem *)));
    connect(ui->cmdOpenFile,SIGNAL(clicked()),this,SLOT(loadLocalFirmware()));
    connect(ui->cmdBack,SIGNAL(clicked()),this,SLOT(backClicked()));
    connect(ui->cmdProgram,SIGNAL(clicked()),this,SLOT(programClicked()));
    connect(ui->cmdCancel,SIGNAL(clicked()),this,SLOT(closeClicked()));
    connect(ui->cmbSource,SIGNAL(currentIndexChanged(int)),this,SLOT(cmbSourceChanged(int)));
    connect(ui->chkShowLog,SIGNAL(stateChanged(int)),this,SLOT(chkShowLogClicked(int)));
    connect(programmer, SIGNAL(readyReadStandardOutput()), this, SLOT(programmerSTDOUTReady()));
    connect(programmer, SIGNAL(readyReadStandardError()), this, SLOT(programmerSTDERRReady()));
    connect(programmer, SIGNAL(started()), this, SLOT(programmerStarted()));
    connect(programmer, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(programmerFinished(int, QProcess::ExitStatus)));
    connect(programmer, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(programmerErrorOccured(QProcess::ProcessError)));

    // Port Discovery
    discoverPortTmr.setInterval(200);
    connect(&discoverPortTmr,&QTimer::timeout,this,&FirmwareWizard::discoverTimeout);
    connect(&lastPortTmr,&QTimer::timeout,this,&FirmwareWizard::portDiscoverTimeout);
    discoverPortTmr.start();

    // Start on fimware selection window
    ui->stkWidget->setCurrentIndex(0);

    // Call discover to get a current list of ports before connecting the signal
    connect(this, &FirmwareWizard::comPortConnected,this, &FirmwareWizard::comPortConnect);
}

FirmwareWizard::~FirmwareWizard()
{
    delete programmer;
    delete programmerlog;
    delete ui;
}


// Reads sources.ini and fill combobox with choices

void FirmwareWizard::initalize()
{
    ui->cmbSource->blockSignals(true);
    // Read Sources INI file + Add to GUI
    QSettings settings("sources.ini",QSettings::IniFormat);
    ui->cmbSource->clear();
    foreach(QString key, settings.childKeys()) {
        ui->cmbSource->addItem(key,settings.value(key));
    }
    ui->cmbSource->addItem("Local File",QString("file://"));
    ui->cmbSource->blockSignals(false);
    // Cause an Update
    ui->cmbSource->setCurrentIndex(0);
    cmbSourceChanged(0);
    setState(PRG_IDLE);
    ui->cmdBack->setVisible(false);
    if(QFileInfo::exists(ui->txtFirmFile->text()))
        ui->cmdProgram->setEnabled(true);
    else
        ui->cmdProgram->setEnabled(false);
    ui->cmdProgram->setText("Program");
    ui->progressBar->setValue(0);
    ui->stkWidget->setCurrentIndex(0);
    boardType = BRD_UNKNOWN;
}

void FirmwareWizard::addToLog(QString l)
{   
    programmerlog->appendPlainText(l.simplified());
    programmerlog->verticalScrollBar()->setValue(programmerlog->verticalScrollBar()->maximum());
}

// Load a file from the local computer, on user click ... button
void FirmwareWizard::loadLocalFirmware()
{
    QString filename = QFileDialog::getOpenFileName(this,"Open File",QString(),"Fimware files (*.hex *.bin)");
    if(!filename.isEmpty()) {
        //localfirmfile = filename; // Save not-encoded
        firmwarefile = "file://" + filename;
        ui->cmdProgram->setEnabled(true);
        ui->txtFirmFile->setText(filename);
    }
    else {
        firmwarefile = "";
        ui->cmdProgram->setEnabled(false);
        ui->txtFirmFile->setText("");
    }
}

void FirmwareWizard::startPortDiscovery(const QString &filename)
{
    // Reset Known Ports
    discoverPorts(true);

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this,"Error","Unable to open firmware file " + filename);
        backClicked();
        ui->stkWidget->setCurrentIndex(0);
        return;
    }

    // Discover File Type
    QByteArray data = file.read(44);
    file.close();

    addToLog("Determining what type of file this is...");

    if(data.startsWith(BLE33HEADER_BIN_MBED) ||
       QFileInfo(file).fileName().startsWith("BLE") ||
       QFileInfo(file).fileName().startsWith("DTQ") ||
       data.mid(2,2) == BLE33HEADER_BIN_ZEPHER) {
        addToLog("  Firmware is for the Arduino Nano BLE 33 in bin format");
        boardType = BRD_NANO33BLE;
        programmercommand = bossac_programmer;
        arguments.clear();
        arguments += "-e";
        arguments += "-w";
        arguments += "-R";
        arguments += filename;
        setState(PRG_WAIT4PORT);
    }
    else if(data.startsWith(BLE33HEADER_HEX)) {
            addToLog("  Firmware is for the Arduino Nano BLE 33 in hex format");
            boardType = BRD_NANO33BLE;
            programmercommand = bossac_programmer;
            arguments.clear();
            arguments += "-e";
            arguments += "-w";
            arguments += "-R";
            arguments += filename;
            setState(PRG_WAIT4PORT);

    } else if (data.startsWith(NANOHEADER_HEX)) {
        QFile::copy(filename,"localavr.hex"); // AVRdude doesn't like paths, so use a local file
        addToLog("  Firmware is for the Arduino Nano in intel hex format");
        boardType = BRD_ARDUINONANO;
        programmercommand = "avrdude.exe";
        arguments.clear();
        arguments += "-patmega328p";
        arguments += "-carduino";
        arguments += "-b115200";
        arguments += "-D";
        arguments += QString("-Uflash:w:localavr.hex:i");
        setState(PRG_WAIT4PORT);
        setState(PRG_SETBOOTLOAD);
        setState(PRG_WAIT4NEWPORT);
    } else {
        QMessageBox::critical(this,"Error", "Unknown firmware type");
        backClicked();
    }
}

void FirmwareWizard::startProgramming()
{
    if(boardType == BRD_NANO33BLE) {
        arguments.prepend(QString("--port=%1").arg(lastFoundPort));
        addToLog("Starting: " + programmercommand + " " + arguments.join(" "));

        programmer->start(programmercommand, arguments);
    } else if (boardType == BRD_ARDUINONANO) {
        arguments.prepend(QString ("-P" + lastFoundPort));
        addToLog("Starting: " + programmercommand + " " + arguments.join(" "));
        programmer->start(programmercommand, arguments);
    }
}

void FirmwareWizard::loadOnlineFirmware()
{
    if(ui->cmbSource->currentIndex()<0)
        return;
    QString sturl = ui->cmbSource->currentData().toString();
    ui->cmdProgram->setEnabled(false);

    // Local File
    if(sturl.startsWith("file://")) {
        parseFirmwareFile(sturl.mid(7));
        ui->stkFirmList->setCurrentIndex(0);

    // Online File
    } else {
        QUrl url(sturl);
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute,false);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,false);
        firmreply = manager.get(request);
        connect(firmreply,SIGNAL(finished()),this,SLOT(firmwareVersionsReady()));
        connect(firmreply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
        connect(firmreply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(firmReplyErrorOccurred(QNetworkReply::NetworkError)));
        ui->stkFirmList->setCurrentIndex(1);
    }
}

void FirmwareWizard::firmwareVersionsReady()
{
    ui->stkFirmList->setCurrentIndex(0);
    // Store the remote file localy
    QByteArray ba;
    QFile file(localfirmlist);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        return;

    ba = firmreply->readAll();
    file.write(ba);
    file.flush();
    file.close();

    parseFirmwareFile(localfirmlist);

    // Delete it
    firmreply->deleteLater();
}

void FirmwareWizard::firmwareReady()
{
    setState(PRG_GOT_FIRMWARE);

    // If loading from local make a copy of the file.
    if(firmwarefile.startsWith("file://")) {
        addToLog("Using firmware from local file " + firmwarefile.mid(7));
        startPortDiscovery(firmwarefile.mid(7));

    // If called from network request complete, store to a local file
    } else {
        addToLog("Downloaded firmware from the internet");
        // Store the remote file localy
        QByteArray ba;
        QFile file(localfirmware);
        file.open(QIODevice::WriteOnly|QIODevice::Truncate);
        ba = hexreply->readAll();
        file.write(ba);
        file.flush();
        file.close();
        startPortDiscovery(localfirmware);

        hexreply->deleteLater();
    }
}

void FirmwareWizard::parseFirmwareFile(QString ff)
{
    // Use QSettings to parse the Ini File
    QSettings ini(ff,QSettings::IniFormat);
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
}

void FirmwareWizard::setState(FirmwareWizard::prgstate state)
{
    programmerState = state;

    switch(programmerState) {
    case PRG_IDLE:
        ui->ledBootloader->setState(false);
        ui->ledComplete->setState(false);
        ui->ledDownloading->setState(false);
        //ui->ledRestoreSettings->setState(false);
        ui->ledUploading->setState(false);
        ui->ledWait4NewPort->setState(false);
        ui->ledWaitPort->setState(false);
        ui->ledBootloader->setBlink(false);
        ui->ledComplete->setBlink(false);
        ui->ledDownloading->setBlink(false);
        //ui->ledRestoreSettings->setBlink(false);
        ui->ledUploading->setBlink(false);
        ui->ledWait4NewPort->setBlink(false);
        ui->ledWaitPort->setBlink(false);
        ui->cmdCancel->setText("Cancel");
        ui->lblState->setText("<html><head/><body><p align='center'><span style=' font-size:18pt;'>Connect your board</span><span style=' font-size:14pt;'><br/></span>If already connected, disconnect and reconnect</p></body></html>");
        break;

    case PRG_REQUEST_FIRMWARE:
        ui->ledDownloading->setBlink(true);
        break;
    case PRG_GOT_FIRMWARE:
        ui->ledDownloading->setState(true);
        break;
    case PRG_WAIT4PORT:
        ui->ledWaitPort->setBlink(true);
        break;
    case PRG_SETBOOTLOAD:
        ui->ledWaitPort->setState(true);
        ui->ledBootloader->setBlink(true);
        waitingprogram=0;
        break;
    case PRG_WAIT4NEWPORT:
        ui->ledBootloader->setState(true);
        ui->ledWait4NewPort->setBlink(true);        
        break;
    case PRG_PROGRAM_START:
        ui->ledWait4NewPort->setState(true);        
        break;
    case PRG_DOWNLOADING:
        ui->ledUploading->setBlink(true);
        break;
    case PRG_PROGRAM_END:
        ui->ledUploading->setState(true);
        break;
    case PRG_CONNECT:
        break;
    case PRG_UPLOADSETTINGS:
        break;
    case PRG_DISCONNECT:
        break;
    case PRG_COMPLETE:
        ui->lblState->setText("<html><head/><body><p align='center'><span style=' font-size:18pt;'>Programming Successful</span><span style=' font-size:14pt;'><br/></span>You may close this window</p></body></html>");
        ui->ledComplete->setState(true);
        ui->cmdCancel->setText("Close");
        ui->cmdProgram->setVisible(false);
        break;
    }
}

void FirmwareWizard::setBootloader()
{
    if(programmerState == PRG_SETBOOTLOAD) {
        if(boardType == BRD_NANO33BLE) {
            // Open and close port at 1200BPS to set bootloader mode
            QSerialPort port(lastFoundPort);
            port.setBaudRate(1200);
            if(!port.open(QIODevice::ReadWrite)) {
                QMessageBox::critical(this,"Error", "Unable to set bootloader, could not open port " + lastFoundPort + " at 1200baud");
                addToLog("Unable to set bootloader - " + port.errorString());

                // Reset
                setState(PRG_IDLE);
                ui->stkWidget->setCurrentIndex(0);
                return;
            }
            port.setRequestToSend(true);
            port.setDataTerminalReady(false);
            QByteArray bootcmd = "\x02{\"Cmd\":\"Boot\"}\xDA\f\x03\r\n";
            port.write(bootcmd,bootcmd.length());
            port.flush();
            port.close();
            setState(PRG_WAIT4NEWPORT);
        }
    }
}

void FirmwareWizard::ssLerrors(const QList<QSslError> &errors)
{
    if(errors.size() > 0)
        QMessageBox::critical(this,"Error",errors.at(0).errorString());
}

void FirmwareWizard::firmReplyErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(firmreply != nullptr) {
        QMessageBox::critical(this,"Fetching Firmwares Error",firmreply->errorString());
        backClicked();
    }
}


void FirmwareWizard::hexReplyErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(hexreply != nullptr){
        QMessageBox::critical(this,"Fetching Binary Error",hexreply->errorString());
        backClicked();
    }
}

/*  Updates the UI to extract the information from the selected firmware
 */
void FirmwareWizard::firmwareSelected(QListWidgetItem *lwi)
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

        if(i.key().toLower().contains("filename"))
            firmwarefile = i.value().toString();

        ui->infoTable->setItem(row,0,text);
        ui->infoTable->setItem(row,1,value);        
    }

    if(!firmwarefile.isEmpty())
        ui->cmdProgram->setEnabled(true);
    ui->infoTable->resizeColumnsToContents();
}

// Start Programming

void FirmwareWizard::programClicked()
{
    if(firmwarefile.isEmpty())
        return;

    // Switch to programming window
    ui->stkWidget->setCurrentIndex(1);
    ui->cmdProgram->setVisible(false);
    ui->cmdBack->setVisible(1);
    ui->ledDownloading->setBlink(true);

    // Clear Log
    programmerlog->clear();
    programmerlog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // If a local file, just call ready
    if(firmwarefile.startsWith("file://")) {
        addToLog("Loading local file " + firmwarefile.mid(7));
        firmwareReady();

    // Otherwise download it
    } else {
        setState(PRG_REQUEST_FIRMWARE);
        QUrl url;

        // Full hostname
        if(firmwarefile.startsWith("http") || firmwarefile.startsWith("ftp")) {
            url.setUrl(firmwarefile);

        // Relative hostname
        } else {
            QStringList host = ui->cmbSource->currentData().toString().split('/',Qt::KeepEmptyParts);
            host.removeLast();
            host.append("");
            QString hostparentfolder = host.join('/');
            url.setUrl(hostparentfolder + firmwarefile);
        }

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute,false);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,false);
        hexreply = manager.get(request);
        connect(hexreply,SIGNAL(finished()),this,SLOT(firmwareReady()));
        connect(hexreply,SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(ssLerrors(const QList<QSslError> &)));
        connect(hexreply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(hexReplyErrorOccurred(QNetworkReply::NetworkError)));
        addToLog("Downloading " + url.toString());
    }
}

void FirmwareWizard::programmerSTDOUTReady()
{
    static QRegularExpression re("\\d+%");

    QByteArray ba = programmer->readAllStandardOutput();
    QString str(ba);

    //[================= ] 58% (35/60 pages)
    // Match one or more digits and a %
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch()) {
      QString strPcnt = match.captured(0);
      strPcnt.chop(1);
      ui->progressBar->setValue(strPcnt.toInt());
    }

    addToLog(ba);
}

void FirmwareWizard::programmerSTDERRReady()
{
    QByteArray ba = programmer->readAllStandardError();
    addToLog(ba);

    // For AVRdude
    if(ba.contains('#')) {
        ui->progressBar->setValue(ui->progressBar->value()+1);
    }
}

void FirmwareWizard::programmerStarted()
{
    setState(PRG_DOWNLOADING);
    addToLog("Starting " + programmercommand + " " + arguments.join(" "));
}

void FirmwareWizard::programmerErrorOccured(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart: {
        QMessageBox::critical(this, "Error", "Unable to open " + programmercommand);        
        backClicked();
        break;
    }
    default: {
        addToLog("Programmer Error " + programmer->errorString());
        backClicked();
    }
    }
}

void FirmwareWizard::programmerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    Q_UNUSED(exitCode);

    if(exitCode == 0) {
        setState(PRG_PROGRAM_END);
        setState(PRG_COMPLETE);
        ui->progressBar->setValue(100);
    } else {
        QMessageBox::critical(this, "Programming Failure", "Programming Failed");
        addToLog(QString("Programmer Exit Error(%1) %2").arg(exitCode).arg(programmer->errorString()));
        backClicked();
    }
}

// Main event timer detecting when to proceed to next state

void FirmwareWizard::discoverTimeout()
{
    if(programmerState == PRG_WAIT4PORT) {
        discoverPorts();
        refreshDelay = 0;
    } else if (programmerState == PRG_WAIT4NEWPORT) {
        waitingprogram++;
        if(waitingprogram > 50) {
            // Waited too long, try the orig port, maybe it's already in bootloader mode?
            addToLog("Wasn't able to set bootloader mode, trying first port found");
            setState(PRG_PROGRAM_START);
            ui->lblERR->setText("Couldn't enter bootloader, trying original port");
            startProgramming();
        }
        else
            discoverPorts();
    } else if (programmerState == PRG_COMPLETE) {
        if(refreshDelay == REFRESH_DELAY) {
            emit programmingComplete();
            refreshDelay++;
        } else if (refreshDelay < REFRESH_DELAY) {
            refreshDelay++;
        }

    }
}

void FirmwareWizard::discoverPorts(bool init)
{
    // Get all current ports
    QStringList curports;
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port,ports) {
        QSerialPort sp(port.portName());
        // Check if it's a usable port by opening it;
        if(sp.open(QIODevice::ReadOnly)){
            curports.append(port.portName());
            sp.flush();
            sp.close();
        }
    }

    // If initializing, don't emit and signals
    if(!init) {
        // Check if a current port isn't contained in the last port list
        foreach(QString port, curports) {
            if(!lastports.contains(port)) {
                // Emit signal in a timer, so only the last detected port gets emitted.
                // BLE board breifly changes com port on startup
                // so depending when timer fires may have grabbed the wrong one
                lastFoundPort = port;
                lastPortTmr.stop();
                lastPortTmr.setInterval(1000);
                lastPortTmr.start();
            }
        }
    }

    // Update list of known ports
    lastports = curports;
}

void FirmwareWizard::comPortConnect(QString comport)
{
    addToLog("Discovered a new port " + comport);
    if(programmerState == PRG_WAIT4PORT) {
        setState(PRG_SETBOOTLOAD);
        QTimer::singleShot(1000,this,SLOT(setBootloader()));
    } else if (programmerState == PRG_WAIT4NEWPORT) {
        // Start programming
        setState(PRG_PROGRAM_START);
        startProgramming();
    }
}

void FirmwareWizard::cmbSourceChanged(int index)
{
    Q_UNUSED(index);
    bool local=false;
    if(ui->cmbSource->currentText() == "Local File")
        local = true;

    ui->cmdOpenFile->setVisible(local);
    ui->cmdProgram->setEnabled(false);
    ui->txtFirmFile->setVisible(local);
    ui->infoTable->setEnabled(!local);
    ui->lstFirmwares->setEnabled(!local);

    if(!local) {
        ui->stkFirmList->setCurrentIndex(1);
        loadOnlineFirmware();
    }
}

void FirmwareWizard::portDiscoverTimeout()
{
    emit comPortConnected(lastFoundPort);
    lastPortTmr.stop();
}

void FirmwareWizard::closeClicked()
{
    backClicked();
    this->hide();
}

void FirmwareWizard::backClicked()
{
    if(programmer->isOpen()) {
        programmer->kill();
        addToLog("\nKilled programmer process!\n");
    }
    setState(PRG_IDLE);
    ui->cmdBack->setVisible(false);
    ui->cmdProgram->setVisible(true);
    ui->progressBar->setValue(0);
    ui->stkWidget->setCurrentIndex(0);
}

void FirmwareWizard::chkShowLogClicked(int state)
{
    if(state == Qt::Checked) {
        programmerlog->show();
        programmerlog->resize(480,480);
        programmerlog->activateWindow();
        programmerlog->raise();
    } else {
        programmerlog->hide();
    }
}

void FirmwareWizard::showEvent(QShowEvent *event)
{
    // Setup Form
    initalize();
    event->accept();
}
