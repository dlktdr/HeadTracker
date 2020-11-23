

#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    // Serial data ready
    connect(serialcon,SIGNAL(readyRead()),this,SLOT(serialReadReady()));

    // UI Data Changed by user
    connect(ui->chkpanrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->spnGyroPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnGyroTilt,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPTiltRoll,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->pan_cnt,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_min,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_max,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->til_cnt,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->til_min,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->til_max,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->til_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_cnt,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_min,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_max,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbpanchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbtiltchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbrllchn,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));

    // LED Timers
    rxledtimer.setInterval(100);
    txledtimer.setInterval(100);
    connect(&rxledtimer,SIGNAL(timeout()),this,SLOT(rxledtimeout()));
    connect(&txledtimer,SIGNAL(timeout()),this,SLOT(txledtimeout()));
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
            qDebug() << "DATA:" << data;
            if(graphing) {
                QStringList xyz = data.split(',');
                if(xyz.length() == 3) {
                    double tilt = xyz.at(0).toDouble();
                    double roll = xyz.at(1).toDouble();
                    double pan = xyz.at(2).toDouble();
                    ui->graphView->addDataPoints(tilt,roll,pan);
                }
            } else {
                // Receive Settings
                if(data.left(5) == QString("$SET$")) {
                    qDebug() << "SETTINGS DATA" << data.right(data.length()-5);

                    QStringList setd = data.right(data.length()-5).split(',',Qt::KeepEmptyParts);
                    //qDebug() << "Leng" << setd.length();

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

    ui->txled->setState(true);
    txledtimer.start();
    serialcon->write(data.toUtf8());
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


    sendSerialData("$VERS");



    sendSerialData("$GSET");
}

// Disconnect from the serial port
void MainWindow::serialDisconnect()
{
    if(serialcon->isOpen())
        serialcon->close();
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
    ui->til_cnt->setValue(trkset.Tlt_cnt());
    ui->til_max->setValue(trkset.Tlt_max());
    ui->til_min->setValue(trkset.Tlt_min());
    ui->til_gain->setValue(trkset.Tlt_gain());

    ui->pan_cnt->setValue(trkset.Pan_cnt());
    ui->pan_max->setValue(trkset.Pan_max());
    ui->pan_min->setValue(trkset.Pan_min());
    ui->pan_gain->setValue(trkset.Pan_gain());

    ui->rll_cnt->setValue(trkset.Rll_cnt());
    ui->rll_max->setValue(trkset.Rll_max());
    ui->rll_min->setValue(trkset.Rll_min());
    ui->rll_gain->setValue(trkset.Rll_gain());

    ui->spnGyroPan->setValue(trkset.gyroWeightPan());
    ui->spnGyroTilt->setValue(trkset.gyroWeightTiltRoll());
    ui->spnLPTiltRoll->setValue(trkset.lpTiltRoll());
    ui->spnLPPan->setValue(trkset.lpPan());

    ui->chkpanrev->setChecked(trkset.panReversed());
    ui->chkrllrev->setChecked(trkset.rollReversed());
    ui->chktltrev->setChecked(trkset.tiltReversed());

    // Combo box don't update on code update, only user update
    ui->cmbpanchn->blockSignals(true); // ?? Better way, couldn't find a signal that isn't only ui changed
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
    trkset.setPan_cnt(ui->pan_cnt->value());
    trkset.setPan_min(ui->pan_min->value());
    trkset.setPan_max(ui->pan_max->value());
    trkset.setPan_gain(ui->pan_gain->value());

    trkset.setTlt_cnt(ui->til_cnt->value());
    trkset.setTlt_min(ui->til_min->value());
    trkset.setTlt_max(ui->til_max->value());
    trkset.setTlt_gain(ui->til_gain->value());

    trkset.setRll_cnt(ui->rll_cnt->value());
    trkset.setRll_min(ui->rll_min->value());
    trkset.setRll_max(ui->rll_max->value());
    trkset.setRll_gain(ui->rll_gain->value());

    trkset.setGyroWeightPan(ui->spnGyroPan->value());
    trkset.setGyroWeightTiltRoll(ui->spnGyroTilt->value());
    trkset.setLPTiltRoll(ui->spnLPTiltRoll->value());
    trkset.setLPPan(ui->spnLPPan->value());

    trkset.setRollReversed(ui->chkrllrev->isChecked());
    trkset.setPanReversed(ui->chkpanrev->isChecked());
    trkset.setTiltReversed(ui->chktltrev->isChecked());

    trkset.setPanCh(ui->cmbpanchn->currentText().toInt());
    trkset.setRollCh(ui->cmbrllchn->currentText().toInt());
    trkset.setTiltCh(ui->cmbtiltchn->currentText().toInt());
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

    if(ui->serialData->toPlainText().length() > 3000)
        ui->serialData->clear();

    ui->serialData->setPlainText(ui->serialData->toPlainText() + QString(sd));

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
}

void MainWindow::stopGraph()
{
    if(!serialcon->isOpen())
        return;
    serialcon->write("$PLEN");
    graphing = false;
}

void MainWindow::uiSettingChanged()
{

}

void MainWindow::storeSettings()
{
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

    qDebug() << data;

    sendSerialData("$" + data + "HE");
    sendSerialData("$SAVE");


}

void MainWindow::rxledtimeout()
{
    ui->rxled->setState(false);
}

void MainWindow::txledtimeout()
{
    ui->txled->setState(false);
}



