
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

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
    bnoCalibratorDialog = new CalibrateBNO;
    ui->statusbar->showMessage("Disconnected");
    findSerialPorts();
    ui->cmdDisconnect->setEnabled(false);
    ui->cmdStopGraph->setEnabled(false);
    ui->cmdStartGraph->setEnabled(false);
    ui->cmdSend->setEnabled(false);
    QFont serifFont("Times", 10, QFont::Bold);

    // Use system default fixed witdh font
    ui->serialData->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    graphing = false;
    xtime = 0;

    // Update default settings to UI
    updateToUI();

    // Serial data ready
    connect(serialcon,SIGNAL(readyRead()),this,SLOT(serialReadReady()));
    connect(serialcon, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),this,SLOT(serialError(QSerialPort::SerialPortError)));

    // Buttons
    connect(ui->cmdConnect,SIGNAL(clicked()),this,SLOT(serialConnect()));
    connect(ui->cmdDisconnect,SIGNAL(clicked()),this,SLOT(serialDisconnect()));    
    connect(ui->cmdStore,SIGNAL(clicked()),this,SLOT(storeSettings()));
    connect(ui->cmdSend,SIGNAL(clicked()),this,SLOT(manualSend()));
    connect(ui->cmdStartGraph,SIGNAL(clicked()),this,SLOT(startGraph()));
    connect(ui->cmdStopGraph,SIGNAL(clicked()),this,SLOT(stopGraph()));
    connect(ui->cmdResetCenter,SIGNAL(clicked()),this, SLOT(resetCenter()));
    connect(ui->cmdCalibrate,SIGNAL(clicked()),this, SLOT(startCalibration()));
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // Check Boxes
    connect(ui->chkpanrev,&QCheckBox::clicked,this,&MainWindow::updateFromUI);
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkRawData,SIGNAL(clicked(bool)),this,SLOT(setDataMode(bool)));

    // Spin Boxes
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

    // On Live Data Change
    connect(&trkset,&TrackerSettings::offOrientChanged,this,&MainWindow::offOrientChanged);
    connect(&trkset,&TrackerSettings::ppmOutChanged,this,&MainWindow::ppmOutChanged);

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
    connect(&acknowledge,SIGNAL(timeout()),this,SLOT(ackTimeout()));

    // Timer to cause an update, prevents too many data writes
    connect(&updatesettingstmr,&QTimer::timeout,this,&MainWindow::updateSettings);

    // Start a timer to tell the device that we are here
    // Times out at 10 seconds so send an ack every 8
    acknowledge.start(8000);
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete firmwareUploader;
    delete bnoCalibratorDialog;
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

        // Strip data up the the CR LF \r\n
        QByteArray data = serialData.left(nlindex);

        // Found a SOT & EOT Character, JSON Data Sent

        if(data.left(1)[0] == (char)0x02 && data.right(1)[0] == (char)0x03) { // JSON Data
            QByteArray stripped = data.mid(1,data.length()-2);
            parseIncomingJSON(QJsonDocument::fromJson(stripped).object().toVariantMap());

        // Found an HT value sent
        } else if(data.left(1) == "$") {
            QRegExp re("$|[A-Z]+"); //***** CHECK IF THIS WORKS!!
            int pos = re.indexIn(data);
            QStringList args = QString(data).mid(pos).split(',',Qt::KeepEmptyParts);
            parseIncomingHT(data.left(pos),args);


            // Other data, Show to the user
        } else {
            addToLog(data + "\n");
        }


        // Remove data read from the buffer
        serialData = serialData.right(serialData.length()-nlindex-2);
    }
}

/* Decide what to do with incoming JSON packet
 * v1.0 Possible Incoming JSON Commands
 *      "GetSet"  Tracker Settings Send
 *      "FW"  Firmware Version
 *      "Data" Live Data for Info / Calibration
 */

void MainWindow::parseIncomingJSON(const QVariantMap &map)
{
    // Settings from the Tracker Sent, save them and update the UI
    if(map["Cmd"].toString() == "Settings") {
        qDebug() << "GOT THE SETTINGS";
        trkset.setAllData(map);
        updateToUI();

    // Data sent, Update the graph / servo sliders / calibration
    } else if (map["Cmd"].toString() == "Data") {
        // Add all the data to the settings
        trkset.setLiveDataMap(map);

    // Firmware Hardware and Version
    } else if (map["Cmd"].toString() == "FW") {
        fwDiscovered(map["Vers"].toString(), map["Hard"].toString());
    }
}

/* fwDiscoverd()
 *      Enables/Disables sections of the GUI based on the hardware found
 */

void MainWindow::fwDiscovered(QString vers, QString hard)
{
    Q_UNUSED(vers);

    // Store in Tracker Settings
    trkset.setHardware(vers,hard);

    // **** CHANGE ME TO USE A DIFFERENT WIDGET FOR EACH HARDWARE ****
    if(hard == "NANO33BLE") {
        ui->cmdStartGraph->setVisible(false);
        ui->cmdStopGraph->setVisible(false);
        ui->cmbRemap->setVisible(false);
        ui->cmbSigns->setVisible(false);



    } else if (hard == "BNO055") {
        ui->cmdStartGraph->setVisible(true);
        ui->cmdStopGraph->setVisible(true);
        ui->cmbRemap->setVisible(true);
        ui->cmbSigns->setVisible(true);


    }
}

/* sendSerialData()
 *      Send Data To The Serial Port
 */

void MainWindow::sendSerialData(QByteArray data)
{
    if(data.isEmpty() || !serialcon->isOpen())
        return;

    addToLog("TX: " + data + "\n");

    ui->txled->setState(true);
    txledtimer.start();
    serialcon->write(data);
}

/* parseIncomingHT()
 *      Read older head tracker code
 */

void MainWindow::parseIncomingHT(QString cmd, QStringList args)
{
    static QString vers;
    static QString hard;

    // CRC ERROR
    if(cmd == "$CRCERR") {
        ui->statusbar->showMessage("CRC Error : Error Setting Values, Retrying",2000);
        updatesettingstmr.start(500);
    }

    // CRC OK
    else if(cmd == "$CRCOK") {
        ui->statusbar->showMessage("Values Set On Headtracker",2000);
    }

    // Calibration Saved
    else if(cmd == "$CALSAV") {
        ui->statusbar->showMessage("Calibration Saved", 2000);
    }

    // Graph Data
    else if(cmd == "$G") {
        if(args.length() == 10) {
            QVariantMap vm;
            vm["tilt"] = args.at(0);
            vm["roll"] = args.at(1);
            vm["pan"] = args.at(2);
            vm["panout"] = args.at(3);
            vm["tiltout"] = args.at(4);
            vm["rollout"] = args.at(5);
            vm["syscal"] = args.at(6);
            vm["gyrocal"] = args.at(7);
            vm["accelcal"] = args.at(8);
            vm["magcal"] = args.at(9);
            trkset.setLiveDataMap(vm);
            graphing = true;
            bnoCalibratorDialog->setCalibration(vm["syscal"].toInt(),
                                             vm["magcal"].toInt(),
                                             vm["gyrocal"].toInt(),
                                             vm["accelcal"].toInt());
        }
    }
    // Setting Data
    else if(cmd == "$SET$") {
        if(args.length() == trkset.count()) {
            trkset.setLPTiltRoll(args.at(0).toFloat());
            trkset.setLPPan(args.at(1).toFloat());
            trkset.setGyroWeightTiltRoll(args.at(2).toFloat());
            trkset.setGyroWeightPan(args.at(3).toFloat());
            trkset.setTlt_gain(args.at(4).toFloat());
            trkset.setPan_gain(args.at(5).toFloat());
            trkset.setRll_gain(args.at(6).toFloat());
            trkset.setServoreverse(args.at(7).toInt());
            trkset.setPan_cnt(args.at(8).toInt());
            trkset.setPan_min(args.at(9).toInt());
            trkset.setPan_max(args.at(10).toInt());
            trkset.setTlt_cnt(args.at(11).toInt());
            trkset.setTlt_min(args.at(12).toInt());
            trkset.setTlt_max(args.at(13).toInt());
            trkset.setRll_cnt(args.at(14).toInt());
            trkset.setRll_min(args.at(15).toInt());
            trkset.setRll_max(args.at(16).toInt());
            trkset.setPanCh(args.at(17).toInt());
            trkset.setTiltCh(args.at(18).toInt());
            trkset.setRollCh(args.at(19).toInt());
            trkset.setAxisRemap(args.at(20).toUInt());
            trkset.setAxisSign(args.at(21).toUInt());

            updateToUI();
            ui->statusbar->showMessage(tr("Settings Received"),2000);
        } else {
            ui->statusbar->showMessage(tr("Error wrong # params"),2000);
        }
    } else if(cmd == "$VERS") {
        if(args.length() == 1) {
            vers = args.at(0);
            if(!hard.isEmpty())
                fwDiscovered(vers,hard);

        }
    } else if(cmd == "$HARD") {
        if(args.length() == 1) {
            hard = args.at(0);
            if(!vers.isEmpty())
                fwDiscovered(vers,hard);
        }
    }
}

/* sendSerialJSON()
 *      Sends a JSON Packet
 */

void MainWindow::sendSerialJSON(QString command, QVariantMap map)
{    
    QJsonObject jobj = QJsonObject::fromVariantMap(map);
    jobj["Cmd"] = command;
    QJsonDocument jdoc(jobj);
    QString json = QJsonDocument(jdoc).toJson(QJsonDocument::Compact);
    sendSerialData((char)0x02 + json.toLatin1() + (char)0x03);

    // Delay this so there is time to process it on the other side
    QTime dieTime= QTime::currentTime().addMSecs(100);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/* addToLog()
 *      Add other information to the LOG
 */

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

    // Setup serial port 8N1, 1M Baud
    serialcon->setPortName(port);
    serialcon->setParity(QSerialPort::NoParity);
    serialcon->setDataBits(QSerialPort::Data8);
    serialcon->setStopBits(QSerialPort::OneStop);
    serialcon->setBaudRate(1000000); // 1 Mega Baud
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

    serialcon->setDataTerminalReady(true);

    QTimer::singleShot(500,this,&MainWindow::requestTimer);
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

void MainWindow::serialError(QSerialPort::SerialPortError err)
{
    switch(err) {
    // Issue with connection - device unplugged
    case QSerialPort::ResourceError: {
        serialDisconnect();
        break;
    }
    default: {
        qDebug() << "SERIAL PORT ERROR" << err;
    }
    }
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

/* offOrientChanged()
 *      New data available for the graph
 */

void MainWindow::offOrientChanged(float t,float r,float p)
{
    ui->graphView->addDataPoints(t,r,p);
}

void MainWindow::ppmOutChanged(int t,int r,int p)
{
    ui->servoTilt->setActualPosition(t);
    ui->servoRoll->setActualPosition(r);
    ui->servoPan->setActualPosition(p);

    // Add a timer here so if no updates these disable
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
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
    /*if(!serialcon->isOpen())
        return;
    serialcon->write("$PLST");
    graphing = true;
    xtime = 0;*/
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

// Send All Settings to the Controller
void MainWindow::storeSettings()
{
    sendSerialJSON("Setttings", trkset.getAllData());
    ui->statusbar->showMessage(tr("Settings Stored to EEPROM"),2000);
}

void MainWindow::updateSettings()
{

}

void MainWindow::resetCenter()
{
    sendSerialJSON("RstCnt");
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
    // Request in HT Mode
    sendSerialData("$VERS");
    sendSerialData("$HARD");
    sendSerialData("$GSET");

    // Request in JSON Mode
    sendSerialJSON("FWR"); // Get the firmware
    sendSerialJSON("GetSet"); // Get the Settings

    ackTimeout(); // Tell it we are here*/
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
    if(serialcon->isOpen()) {
        QMessageBox::information(this,"Cannot Upload", "Disconnect before uploading a new firmware");
    } else {
        firmwareUploader->show();
        firmwareUploader->activateWindow();
        firmwareUploader->raise();
        firmwareUploader->setComPort(ui->cmdPort->currentText());
    }
}

// Start the various calibration dialogs

void MainWindow::startCalibration()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this,"Not Connected", "Connect before trying to calibrate");
        return;
    }

    if(trkset.getHardware() == "NANO33BLE") {
        // TODO

    } else if (trkset.getHardware() == "BNO055") {
        // Start calibration, start graphing.
        sendSerialData("$STO");
        startGraph();
        bnoCalibratorDialog->startCalibration();
        bnoCalibratorDialog->show();

    }
}

// Nano33BLE - Let Hardware know were here

void MainWindow::ackTimeout()
{
    if(serialcon->isOpen() && trkset.getHardware() == "NANO33BLE") {
        sendSerialJSON("ACK");
    }
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QCoreApplication::quit();
    event->accept();
}

