

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servominmax.h"
#include "ucrc16lib.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    firmwareUploader = new Firmware;
    serialcon = new QSerialPort;
    ui->statusbar->showMessage("Disconnected");
    findSerialPorts();
    ui->cmdDisconnect->setEnabled(false);
    ui->cmdStopGraph->setEnabled(false);
    ui->cmdStartGraph->setEnabled(false);
    ui->cmdSend->setEnabled(false);
    QFont serifFont("Times", 10, QFont::Bold);
    // Use system default fixed with font
    ui->serialData->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    graphing = false;
    xtime = 0;
    updateToUI();

    // Serial data ready
    connect(serialcon,SIGNAL(readyRead()),this,SLOT(serialReadReady()));

    // Buttons
    connect(ui->cmdConnect,SIGNAL(clicked()),this,SLOT(serialConnect()));
    connect(ui->cmdDisconnect,SIGNAL(clicked()),this,SLOT(serialDisconnect()));    
    connect(ui->cmdStore,SIGNAL(clicked()),this,SLOT(storeSettings()));
    connect(ui->cmdSend,SIGNAL(clicked()),this,SLOT(manualSend()));
    connect(ui->cmdStartGraph,SIGNAL(clicked()),this,SLOT(startGraph()));
    connect(ui->cmdStopGraph,SIGNAL(clicked()),this,SLOT(stopGraph()));
    connect(ui->cmdResetCenter,SIGNAL(clicked()),this, SLOT(resetCenter()));
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // Check Boxes
    connect(ui->chkpanrev,&QCheckBox::clicked,this,&MainWindow::updateFromUI);
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkRawData,SIGNAL(clicked(bool)),this,SLOT(setDataMode(bool)));
    //connect(ui->spnGyroPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    //connect(ui->spnGyroTilt,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPTiltRoll,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));

    // Gain Sliders
    connect(ui->til_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));

    // Servo Scaling Widgets
    connect(ui->servoPan,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoPan,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoPan,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);

    // Combo Boxes

    // Add Remap Choices + The corresponding values
    ui->cmbRemap->addItem("X,Y,Z",AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z));
    ui->cmbRemap->addItem("X,Z,Y",AXES_MAP(AXIS_X,AXIS_Z,AXIS_Y));
    ui->cmbRemap->addItem("Y,X,Z",AXES_MAP(AXIS_Y,AXIS_X,AXIS_Z));
    ui->cmbRemap->addItem("Y,Z,X",AXES_MAP(AXIS_Y,AXIS_Z,AXIS_X));
    ui->cmbRemap->addItem("Z,X,Y",AXES_MAP(AXIS_Z,AXIS_X,AXIS_Y));
    ui->cmbRemap->addItem("Z,Y,Z",AXES_MAP(AXIS_Z,AXIS_Y,AXIS_X));
    ui->cmbRemap->setCurrentIndex(0);

    connect(ui->cmbpanchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbtiltchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbrllchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbRemap,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbSigns,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));

    // Menu Actions
    connect(ui->action_Save_to_File,SIGNAL(triggered()),this,SLOT(saveSettings()));
    connect(ui->action_Load,SIGNAL(triggered()),this,SLOT(loadSettings()));
    connect(ui->actionE_xit,SIGNAL(triggered()),QCoreApplication::instance(),SLOT(quit()));
    connect(ui->actionUpload_Firmware,SIGNAL(triggered()),this,SLOT(uploadFirmwareClick()));

    // LED Timers
    rxledtimer.setInterval(100);
    txledtimer.setInterval(100);
    connect(&rxledtimer,SIGNAL(timeout()),this,SLOT(rxledtimeout()));
    connect(&txledtimer,SIGNAL(timeout()),this,SLOT(txledtimeout()));    

    // Timer to cause an update, prevents too many data writes
    connect(&updatesettingstmr,&QTimer::timeout,this,&MainWindow::updateSettings);
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete firmwareUploader;
    delete ui;

}

// Parse the data received from the serial port
void MainWindow::parseSerialData()
{      
    bool done = true;
    while(done) {

            int nlindex = serialData.indexOf("\r\n");
            if(nlindex < 0)
                return;  // No New line found
            QString data = serialData.left(nlindex);

            // CRC ERROR
            if(data.left(7) == QString("$CRCERR")) {
                ui->statusbar->showMessage("CRC Error : Error Setting Values, Retrying",2000);
                updatesettingstmr.start(500);
            }

            // CRC OK
             else if(data.left(6) == QString("$CRCOK")) {
                ui->statusbar->showMessage("Values Set On Headtracker",2000);
            }

            // Graph Data
            else if(data.left(2) == QString("$G")) {
                data = data.mid(2).simplified();
                QStringList rtd = data.split(',');
                if(rtd.length() == 10) {
                    double tilt = rtd.at(0).toDouble();
                    double roll = rtd.at(1).toDouble();
                    double pan = rtd.at(2).toDouble();
                    int panout = rtd.at(3).toInt();
                    int tiltout = rtd.at(4).toInt();
                    int rollout = rtd.at(5).toInt();
                    int syscal = rtd.at(6).toInt();
                    int gyrocal = rtd.at(7).toInt();
                    int accelcal = rtd.at(8).toInt();
                    int magcal = rtd.at(9).toInt();
                    ui->graphView->addDataPoints(tilt,roll,pan);
                    ui->servoPan->setActualPosition(panout);
                    ui->servoTilt->setActualPosition(tiltout);
                    ui->servoRoll->setActualPosition(rollout);
                    ui->sbs->setSignal(syscal);
                    ui->sba->setSignal(accelcal);
                    ui->sbm->setSignal(magcal);
                    ui->sbg->setSignal(gyrocal);
                    graphing = true;
                    ui->servoPan->setShowActualPosition(true);
                    ui->servoTilt->setShowActualPosition(true);
                    ui->servoRoll->setShowActualPosition(true);
                }
            }

            // Data Receieve
            else if(data.left(5) == QString("$SET$")) {
                QStringList setd = data.right(data.length()-5).split(',',Qt::KeepEmptyParts);

                if(setd.length() == trkset.count()) {
                    trkset.setLPTiltRoll(setd.at(0).toFloat());
                    trkset.setLPPan(setd.at(1).toFloat());
                    trkset.setGyroWeightTiltRoll(setd.at(2).toFloat());
                    trkset.setGyroWeightPan(setd.at(3).toFloat());
                    trkset.setTlt_gain(setd.at(4).toFloat());
                    trkset.setPan_gain(setd.at(5).toFloat());
                    trkset.setRll_gain(setd.at(6).toFloat());
                    trkset.setServoreverse(setd.at(7).toInt());
                    trkset.setPan_cnt(setd.at(8).toInt());
                    trkset.setPan_min(setd.at(9).toInt());
                    trkset.setPan_max(setd.at(10).toInt());
                    trkset.setTlt_cnt(setd.at(11).toInt());
                    trkset.setTlt_min(setd.at(12).toInt());
                    trkset.setTlt_max(setd.at(13).toInt());
                    trkset.setRll_cnt(setd.at(14).toInt());
                    trkset.setRll_min(setd.at(15).toInt());
                    trkset.setRll_max(setd.at(16).toInt());
                    trkset.setPanCh(setd.at(17).toInt());
                    trkset.setTiltCh(setd.at(18).toInt());
                    trkset.setRollCh(setd.at(19).toInt());
                    trkset.setAxisRemap(setd.at(20).toUInt());
                    trkset.setAxisSign(setd.at(21).toUInt());

                    updateToUI();
                    ui->statusbar->showMessage(tr("Settings Received"),2000);
                } else {
                    ui->statusbar->showMessage(tr("Error wrong # params"),2000);
                }
            }

            // Other information, Log it
            else {
                addToLog(data + "\n");
            }

            serialData = serialData.right(serialData.length()-nlindex-2);
    }
}

void MainWindow::sendSerialData(QByteArray data)
{
    if(data.isEmpty() || !serialcon->isOpen())
        return;

    addToLog("TX: " + data + "\n");

    ui->txled->setState(true);
    txledtimer.start();
    serialcon->write(data);
}

void MainWindow::addToLog(QString log)
{
    // Add information to the Log
    logd += log;

    // Limit Max Log Length
    int loglen = logd.length();
    if(loglen > MAX_LOG_LENGTH)
        logd = logd.mid(logd.length() - MAX_LOG_LENGTH);

    // Set Gui text from local string
    ui->serialData->setPlainText(logd);

    // Scroll to bottom
    ui->serialData->verticalScrollBar()->setValue(ui->serialData->verticalScrollBar()->maximum());
}

// Finds available serial ports
void MainWindow::findSerialPorts()
{
    ui->cmdPort->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port,ports) {
        ui->cmdPort->addItem(port.portName(),port.serialNumber());
    }
}

// Connects to the serial port
void MainWindow::serialConnect()
{
    QString port = ui->cmdPort->currentText();
    if(port.isEmpty())
        return;
    if(serialcon->isOpen())
        serialcon->close();

    // Setup serial port 8N1, 57600 BAUD
    serialcon->setPortName(port);
    serialcon->setParity(QSerialPort::NoParity);
    serialcon->setDataBits(QSerialPort::Data8);
    serialcon->setStopBits(QSerialPort::OneStop);
    serialcon->setBaudRate(QSerialPort::Baud57600);
    serialcon->setFlowControl(QSerialPort::NoFlowControl);

    if(!serialcon->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this,"Error",tr("Could not open Com ") + serialcon->portName());
        return;
    }

    logd.clear();
    ui->serialData->clear();

    ui->cmdDisconnect->setEnabled(true);
    ui->cmdConnect->setEnabled(false);
    ui->cmdStopGraph->setEnabled(true);
    ui->cmdStartGraph->setEnabled(true);
    ui->cmdSend->setEnabled(true);

    ui->statusbar->showMessage(tr("Connected to ") + serialcon->portName());

    QTimer::singleShot(2000,this,&MainWindow::requestTimer);
}

// Disconnect from the serial port
void MainWindow::serialDisconnect()
{
    if(serialcon->isOpen()) {
        if(graphing)
            stopGraph();
        serialcon->flush();
        serialcon->close();
    }
    ui->statusbar->showMessage(tr("Disconnected"));
    ui->cmdDisconnect->setEnabled(false);
    ui->cmdConnect->setEnabled(true);
    ui->cmdStopGraph->setEnabled(false);
    ui->cmdStartGraph->setEnabled(false);
    ui->cmdSend->setEnabled(false);
}

// Update the UI Settings from the settings class
void MainWindow::updateToUI()
{
    ui->servoTilt->setCenter(trkset.Tlt_cnt());
    ui->servoTilt->setMaximum(trkset.Tlt_max());
    ui->servoTilt->setMinimum(trkset.Tlt_min());

    ui->servoPan->setCenter(trkset.Pan_cnt());
    ui->servoPan->setMaximum(trkset.Pan_max());
    ui->servoPan->setMinimum(trkset.Pan_min());

    ui->servoRoll->setCenter(trkset.Rll_cnt());
    ui->servoRoll->setMaximum(trkset.Rll_max());
    ui->servoRoll->setMinimum(trkset.Rll_min());

    ui->til_gain->setValue(trkset.Tlt_gain());
    ui->pan_gain->setValue(trkset.Pan_gain());
    ui->rll_gain->setValue(trkset.Rll_gain());

    ui->spnLPTiltRoll->setValue(trkset.lpTiltRoll());
    ui->spnLPPan->setValue(trkset.lpPan());

    ui->chkpanrev->setChecked(trkset.isPanReversed());
    ui->chkrllrev->setChecked(trkset.isRollReversed());
    ui->chktltrev->setChecked(trkset.isTiltReversed());

    ui->cmbpanchn->blockSignals(true);
    ui->cmbrllchn->blockSignals(true);
    ui->cmbtiltchn->blockSignals(true);
    ui->cmbRemap->blockSignals(true);

    ui->cmbpanchn->setCurrentIndex(trkset.panCh()-1);
    ui->cmbrllchn->setCurrentIndex(trkset.rollCh()-1);
    ui->cmbtiltchn->setCurrentIndex(trkset.tiltCh()-1);
    ui->cmbRemap->setCurrentIndex(ui->cmbRemap->findData(trkset.axisRemap()));
    ui->cmbSigns->setCurrentIndex(trkset.axisSign());

    ui->cmbpanchn->blockSignals(false);
    ui->cmbrllchn->blockSignals(false);
    ui->cmbtiltchn->blockSignals(false);
    ui->cmbRemap->blockSignals(false);

}

// Update the Settings Class from the UI Data
void MainWindow::updateFromUI()
{
    trkset.setPan_cnt(ui->servoPan->centerValue());
    trkset.setPan_min(ui->servoPan->minimumValue());
    trkset.setPan_max(ui->servoPan->maximumValue());
    trkset.setPan_gain(ui->pan_gain->value());

    trkset.setTlt_cnt(ui->servoTilt->centerValue());
    trkset.setTlt_min(ui->servoTilt->minimumValue());
    trkset.setTlt_max(ui->servoTilt->maximumValue());
    trkset.setTlt_gain(ui->til_gain->value());

    trkset.setRll_cnt(ui->servoRoll->centerValue());
    trkset.setRll_min(ui->servoRoll->minimumValue());
    trkset.setRll_max(ui->servoRoll->maximumValue());
    trkset.setRll_gain(ui->rll_gain->value());

    trkset.setLPTiltRoll(ui->spnLPTiltRoll->value());
    trkset.setLPPan(ui->spnLPPan->value());

    trkset.setRollReversed(ui->chkrllrev->isChecked());
    trkset.setPanReversed(ui->chkpanrev->isChecked());
    trkset.setTiltReversed(ui->chktltrev->isChecked());

    trkset.setPanCh(ui->cmbpanchn->currentText().toInt());
    trkset.setRollCh(ui->cmbrllchn->currentText().toInt());
    trkset.setTiltCh(ui->cmbtiltchn->currentText().toInt());

    trkset.setAxisRemap(ui->cmbRemap->currentData().toUInt());
    trkset.setAxisSign(ui->cmbSigns->currentIndex());

    updatesettingstmr.start(1000);
}

// Data ready to be read from the serial port
void MainWindow::serialReadReady()
{
    // Read all serial data
    bool scroll = false;
    QByteArray sd = serialcon->readAll();
    serialData.append(sd);

    int slider = ui->serialData->verticalScrollBar()->value();
    if(slider == ui->serialData->verticalScrollBar()->maximum())
        scroll = true; 

    // Scroll to bottom
    if(scroll)
        ui->serialData->verticalScrollBar()->setValue(ui->serialData->verticalScrollBar()->maximum());
    else
        ui->serialData->verticalScrollBar()->setValue(slider);

    parseSerialData();

    ui->rxled->setState(true);
    rxledtimer.start();
}

void MainWindow::manualSend()
{
    sendSerialData(ui->txtCommand->text().toLatin1());
}

void MainWindow::startGraph()
{
    if(!serialcon->isOpen())
        return;
    serialcon->write("$PLST");
    graphing = true;
    xtime = 0;
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
}

void MainWindow::stopGraph()
{
    if(!serialcon->isOpen())
        return;
    serialcon->write("$PLEN");
    graphing = false;
    ui->servoPan->setShowActualPosition(false);
    ui->servoTilt->setShowActualPosition(false);
    ui->servoRoll->setShowActualPosition(false);
}

void MainWindow::storeSettings()
{
    ui->statusbar->showMessage(tr("Settings Stored to EEPROM"),2000);
    sendSerialData("$SAVE");
}

void MainWindow::updateSettings()
{
    // Disable Graphing output, all the extra data was causing issues
    // on update
    if(graphing) {
        serialcon->write("$PLEN");
    }

    updatesettingstmr.stop();
    QStringList lst;
    lst.append(QString::number(trkset.lpTiltRoll()));
    lst.append(QString::number(trkset.lpPan()));
    lst.append(QString::number(trkset.gyroWeightTiltRoll()));
    lst.append(QString::number(trkset.gyroWeightPan()));
    lst.append(QString::number(trkset.Tlt_gain()));
    lst.append(QString::number(trkset.Pan_gain()));
    lst.append(QString::number(trkset.Rll_gain()));
    lst.append(QString::number(trkset.servoReverse()));
    lst.append(QString::number(trkset.Pan_cnt()));
    lst.append(QString::number(trkset.Pan_min()));
    lst.append(QString::number(trkset.Pan_max()));
    lst.append(QString::number(trkset.Tlt_cnt()));
    lst.append(QString::number(trkset.Tlt_min()));
    lst.append(QString::number(trkset.Tlt_max()));
    lst.append(QString::number(trkset.Rll_cnt()));
    lst.append(QString::number(trkset.Rll_min()));
    lst.append(QString::number(trkset.Rll_max()));
    lst.append(QString::number(trkset.panCh()));
    lst.append(QString::number(trkset.tiltCh()));
    lst.append(QString::number(trkset.rollCh()));
    lst.append(QString::number(trkset.axisRemap()));
    lst.append(QString::number(trkset.axisSign()));
    QString data = lst.join(',');

    // Calculate the CRC Checksum
    uint16_t CRC = uCRC16Lib::calculate(data.toUtf8().data(),data.length());

    // Append Data in a Byte Array
    QByteArray bd = QString("$" + data).toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + "HE";

    sendSerialData(bd);

    // Re-Enable Graphing if required
    if(graphing) {
        serialcon->write("$PLST");
    }
}

void MainWindow::resetCenter()
{
    // Reset Values to Center
    sendSerialData("$RST ");
}

void MainWindow::setDataMode(bool rawmode)
{
    // Change mode to show offset vs raw unfiltered data
    if(rawmode)
        sendSerialData("$GRAW ");
    else
        sendSerialData("$GOFF ");
}

void MainWindow::requestTimer()
{
    sendSerialData("$VERS ");
    sendSerialData("$GSET ");
}

void MainWindow::rxledtimeout()
{
    ui->rxled->setState(false);
}

void MainWindow::txledtimeout()
{
    ui->txled->setState(false);
}

void MainWindow::saveSettings()
{
    QString filename = QFileDialog::getSaveFileName(this,tr("Save Settings"),QString(),"Config Files (*.ini)");
    if(!filename.isEmpty()) {
        QSettings settings(filename,QSettings::IniFormat);
        trkset.storeSettings(&settings);
    }
}

void MainWindow::loadSettings()
{
    QString filename = QFileDialog::getOpenFileName(this,tr("Open File"),QString(),"Config Files (*.ini)");
    if(!filename.isEmpty()) {
        QSettings settings(filename,QSettings::IniFormat);
        trkset.loadSettings(&settings);
        updateToUI();
        updateSettings();
    }
}

void MainWindow::uploadFirmwareClick()
{
    firmwareUploader->show();
    firmwareUploader->activateWindow();
    firmwareUploader->raise();
    firmwareUploader->setComPort(ui->cmdPort->currentText());
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QCoreApplication::quit();
    event->accept();
}

