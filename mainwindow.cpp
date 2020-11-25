

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servominmax.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serialcon = new QSerialPort;
    ui->statusbar->showMessage("Disconnected");
    findSerialPorts();
    ui->cmdDisconnect->setEnabled(false);
    ui->cmdStopGraph->setEnabled(false);
    ui->cmdStartGraph->setEnabled(false);
    ui->cmdSend->setEnabled(false);
    QFont serifFont("Times", 10, QFont::Bold);
    graphing = false;
    xtime = 0;
    updateToUI();

    // Buttons
    connect(ui->cmdConnect,SIGNAL(clicked()),this,SLOT(serialConnect()));
    connect(ui->cmdDisconnect,SIGNAL(clicked()),this,SLOT(serialDisconnect()));    
    connect(ui->cmdStore,SIGNAL(clicked()),this,SLOT(storeSettings()));
    connect(ui->cmdSend,SIGNAL(clicked()),this,SLOT(manualSend()));
    connect(ui->cmdStartGraph,SIGNAL(clicked()),this,SLOT(startGraph()));
    connect(ui->cmdStopGraph,SIGNAL(clicked()),this,SLOT(stopGraph()));
    connect(ui->cmdResetCenter,SIGNAL(clicked()),this, SLOT(resetCenter()));

    // Serial data ready
    connect(serialcon,SIGNAL(readyRead()),this,SLOT(serialReadReady()));

    // UI Data Changed by user
    connect(ui->chkpanrev,&QCheckBox::clicked,this,&MainWindow::updateFromUI);
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    //connect(ui->spnGyroPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    //connect(ui->spnGyroTilt,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPTiltRoll,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));

    connect(ui->til_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));

    connect(ui->servoPan,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoPan,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoPan,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoTilt,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::minimumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::maximumChanged,this,&MainWindow::updateFromUI);
    connect(ui->servoRoll,&ServoMinMax::centerChanged,this,&MainWindow::updateFromUI);

    connect(ui->cmbpanchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbtiltchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbrllchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // LED Timers
    rxledtimer.setInterval(100);
    txledtimer.setInterval(100);
    connect(&rxledtimer,SIGNAL(timeout()),this,SLOT(rxledtimeout()));
    connect(&txledtimer,SIGNAL(timeout()),this,SLOT(txledtimeout()));
    connect(&updatesettingstmr,&QTimer::timeout,this,&MainWindow::updateSettings);
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete ui;
}

// Parse the data received from the serial port
void MainWindow::parseSerialData()
{      
    //qDebug() << "RX:" << serialData;
    bool done = true;
    while(done) {
            int nlindex = serialData.indexOf("\r\n");
            if(nlindex < 0)
                return;  // No New line found
            QString data = serialData.left(nlindex).simplified();
           // qDebug() << "DATA:" << data;
            if(graphing) {
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
                }
            } else {
                addToLog(data);
                // Receive Settings
                if(data.left(5) == QString("$SET$")) {
           //         qDebug() << "SETTINGS DATA" << data.right(data.length()-5);

                    QStringList setd = data.right(data.length()-5).split(',',Qt::KeepEmptyParts);
                    //qDebug() << "Leng" << setd.length();

                    ui->statusbar->showMessage(tr("Settings Received"));

                    if(setd.length() == 20) {
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
                        updateToUI();
                    } else {
                        qDebug() << "Wrong number of params";
                    }
                }
            }
            serialData = serialData.right(serialData.length()-nlindex-2);
    }
}

void MainWindow::sendSerialData(QString data)
{
    if(data.isEmpty() || !serialcon->isOpen())
        return;

    addToLog(data + "\n");

    ui->txled->setState(true);
    txledtimer.start();
    serialcon->write(data.toUtf8());
}

void MainWindow::addToLog(QString log, bool bold)
{
    logd += log;

    // Limit Max Log Length
    int loglen = logd.length();
    if(loglen > MAX_LOG_LENGTH)
        logd = logd.mid(logd.length() - MAX_LOG_LENGTH);

    // Set Gui
    ui->serialData->setPlainText(logd);

    // Scroll to bottom
    ui->serialData->verticalScrollBar()->setValue(ui->serialData->verticalScrollBar()->maximum());
}

// Find available serial ports
void MainWindow::findSerialPorts()
{
    ui->cmdPort->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port,ports) {
        ui->cmdPort->addItem(port.portName(),port.serialNumber());
    }
}

// Connect to the serial port
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
        QMessageBox::critical(this,"Error",tr("Could not open Com Port ") + serialcon->portName());
        return;
    }

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

    ui->chkpanrev->setChecked(trkset.panReversed());
    ui->chkrllrev->setChecked(trkset.rollReversed());
    ui->chktltrev->setChecked(trkset.tiltReversed());

    ui->cmbpanchn->blockSignals(true);
    ui->cmbrllchn->blockSignals(true);
    ui->cmbtiltchn->blockSignals(true);

    ui->cmbpanchn->setCurrentIndex(trkset.panCh()-1);
    ui->cmbrllchn->setCurrentIndex(trkset.rollCh()-1);
    ui->cmbtiltchn->setCurrentIndex(trkset.tiltCh()-1);

    ui->cmbpanchn->blockSignals(false);
    ui->cmbrllchn->blockSignals(false);
    ui->cmbtiltchn->blockSignals(false);

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

    updatesettingstmr.start(1000);
}

// Data ready to be read from the serial port
void MainWindow::serialReadReady()
{
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
    sendSerialData(ui->txtCommand->text());
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
    ui->statusbar->showMessage(tr("Settings Stored to EEPROM"));
    sendSerialData("$SAVE");
}

void MainWindow::updateSettings()
{
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
    QString data = lst.join(',');

 //   qDebug() << data;

    sendSerialData("$" + data + "HE");

}

void MainWindow::resetCenter()
{
    sendSerialData("$RST");
}

void MainWindow::requestTimer()
{
    //sendSerialData("$VERS");
     sendSerialData("$GSET");
  //   qDebug() << "Requested settings";

}

void MainWindow::rxledtimeout()
{
    ui->rxled->setState(false);
}

void MainWindow::txledtimeout()
{
    ui->txled->setState(false);
}



