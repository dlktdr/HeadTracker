
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

    setWindowTitle(windowTitle() + " " + version);

    // Serial Connection
    serialcon = new QSerialPort;

    // Calibrator Dialogs
    bnoCalibratorDialog = new CalibrateBNO;
    bleCalibratorDialog = new CalibrateBLE(&trkset);

    // Diagnostic Display
    diagnostic = new DiagnosticDisplay(&trkset);
    serialDebug = new QPlainTextEdit();
    serialDebug->setWindowTitle("Serial Information");
    serialDebug->resize(600,300);

    // Hide these buttons until connected
    ui->cmdStartGraph->setVisible(false);
    ui->cmdStopGraph->setVisible(false);
    ui->chkRawData->setVisible(false);

    // Firmware loader loader dialog
    firmwareUploader = new Firmware;

    // Called to initalize GUI state to disabled
    graphing = false;
    serialDisconnect();

    // Get list of available ports
    findSerialPorts();

    // Use system default fixed witdh font
    QFont serifFont("Times", 10, QFont::Bold);
    serialDebug->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

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
    connect(ui->cmdSaveNVM,&QPushButton::clicked,this, &MainWindow::saveToNVM);
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // Check Boxes
    connect(ui->chkpanrev,&QCheckBox::clicked,this,&MainWindow::updateFromUI);
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkInvertedPPM,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkInvertedPPMIn,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkResetCenterWave,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkRawData,SIGNAL(clicked(bool)),this,SLOT(setDataMode(bool)));

    // Spin Boxes
    //connect(ui->spnGyroPan,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    //connect(ui->spnGyroTilt,SIGNAL(editingFinished()),this,SLOT(updateFromUI()));
    connect(ui->spnLPPan,SIGNAL(valueChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->spnLPTiltRoll,SIGNAL(valueChanged(int)),this,SLOT(updateFromUI()));

    // Gain Sliders
    connect(ui->til_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->rll_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    connect(ui->pan_gain,SIGNAL(sliderMoved(int)),this,SLOT(updateFromUI()));
    ui->til_gain->setMaximum(TrackerSettings::MAX_GAIN*10);
    ui->rll_gain->setMaximum(TrackerSettings::MAX_GAIN*10);
    ui->pan_gain->setMaximum(TrackerSettings::MAX_GAIN*10);

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
    connect(&trkset,&TrackerSettings::rawOrientChanged,this,&MainWindow::offOrientChanged);
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
    connect(ui->cmbButtonPin,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbPpmInPin,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbPpmOutPin,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbBtMode,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbOrientation,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));
    connect(ui->cmbResetOnPPM,SIGNAL(currentIndexChanged(int)),this,SLOT(updateFromUI()));

    // Menu Actions
    connect(ui->action_Save_to_File,SIGNAL(triggered()),this,SLOT(saveSettings()));
    connect(ui->action_Load,SIGNAL(triggered()),this,SLOT(loadSettings()));
    connect(ui->actionE_xit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionUpload_Firmware,SIGNAL(triggered()),this,SLOT(uploadFirmwareClick()));
    connect(ui->actionShow_Data,SIGNAL(triggered()),this,SLOT(showDiagsClicked()));
    connect(ui->actionShow_Serial_Transmissions,SIGNAL(triggered()),this,SLOT(showSerialDiagClicked()));

    // Timers
    rxledtimer.setInterval(100);
    txledtimer.setInterval(100);
    connect(&rxledtimer,SIGNAL(timeout()),this,SLOT(rxledtimeout()));
    connect(&txledtimer,SIGNAL(timeout()),this,SLOT(txledtimeout()));
    connect(&imheretimout,SIGNAL(timeout()),this,SLOT(ihTimeout()));
    connect(&connectTimer,SIGNAL(timeout()),this,SLOT(connectTimeout()));
    connectTimer.setSingleShot(true);

    // On BLE Calibration Save update to device
    connect(bleCalibratorDialog,&CalibrateBLE::calibrationSave,this,&MainWindow::storeSettings);

    // Timer to cause an update, prevents too many data writes
    connect(&updatesettingstmr,&QTimer::timeout,this,&MainWindow::updateSettings);
    updatesettingstmr.setSingleShot(true);

    // Communication timer. Checks for lack of ACK & NAK codes
    connect(&comtimeout,&QTimer::timeout,this,&MainWindow::comTimeout);
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete firmwareUploader;
    delete bnoCalibratorDialog;
    delete bleCalibratorDialog;
    delete serialDebug;
    delete ui;
}

// Connects to the serial port
void MainWindow::serialConnect()
{
    QString port = ui->cmbPort->currentText();
    if(port.isEmpty())
        return;
    if(serialcon->isOpen())
        serialcon->close();

    // Setup serial port 8N1, 57600 Baud
    serialcon->setPortName(port);
    serialcon->setParity(QSerialPort::NoParity);
    serialcon->setDataBits(QSerialPort::Data8);
    serialcon->setStopBits(QSerialPort::OneStop);
    serialcon->setBaudRate(QSerialPort::Baud115200); // CDC doesn't actuall make a dif.. cool
    serialcon->setFlowControl(QSerialPort::NoFlowControl);

    if(!serialcon->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this,"Error",tr("Could not open Com ") + serialcon->portName());
        return;
    }

    logd.clear();
    serialDebug->clear();
    trkset.clear();

    ui->cmdDisconnect->setEnabled(true);
    ui->cmdConnect->setEnabled(false);


    ui->statusbar->showMessage(tr("Connected to ") + serialcon->portName());

    serialcon->setDataTerminalReady(true);

    ui->stackedWidget->setCurrentIndex(1);
    addToLog("Waiting for board to boot...\n");
    QTimer::singleShot(4000,this,&MainWindow::requestTimer);

    fwdiscovered = false;
}

// Disconnect from the serial port
// Reset all gui settings to disable mode
void MainWindow::serialDisconnect()
{
    connectTimer.stop();

    if(serialcon->isOpen()) {
        // Check if user wants to save first
        if(!checkSaved())
            return;
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
    ui->cmdSaveNVM->setEnabled(false);
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
    ui->cmdSend->setEnabled(false);
    ui->cmdCalibrate->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
    bleCalibratorDialog->hide();
    bnoCalibratorDialog->hide();

    fwdiscovered = false;
    calmsgshowed = false;
    savedToNVM = true;
    sentToHT = true;
    rawmode = false;
    jsonfaults = 0;
    comtimeout.stop();
    updatesettingstmr.stop();
    imheretimout.stop();
    jsonqueue.clear();

    connectTimer.stop();
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

        // Return if only a \r\n, no data.
        if(data.length() < 1) {
            serialData = serialData.right(serialData.length()-nlindex-2);
            return;
        }

        // Found a SOT & EOT Character. JSON Data was sent
        if(data.left(1)[0] == (char)0x02 && data.right(1)[0] == (char)0x03) { // JSON Data
            QByteArray stripped = data.mid(1,data.length()-2);

            // *** TODO - Add CRC check here, issue a new settings request.
            // Increment RXerror counter and fault out if too many errors.

            parseIncomingJSON(QJsonDocument::fromJson(stripped).object().toVariantMap());            

        //  Found the acknowldege Character, data was received without error
        } else if(data.left(1)[0] == (char)0x06) {
            // Clear the fault counter
            jsonfaults = 0;
            comtimeout.stop();

            // If more data in queue, send it and wait for another ack char.
            if(!jsonqueue.isEmpty()) {
                sendSerialData(jsonqueue.dequeue());
                jsonfaults = 1;
            }

        // Found a not-acknowldege character, resend data
        } else if(data.left(1)[0] == (char)0x15) {
            comTimeout();

        // Found an HT value sent
        } else if(data.left(1) == "$") {
            parseIncomingHT(data);

        // Other data sent, show the user
        } else {
            addToLog(data + "\n");
        }

        // Remove data read from the buffer
        serialData = serialData.right(serialData.length()-nlindex-2);
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

    // Stack widget changes to hide some info depending on board
    if(hard == "NANO33BLE") {
        ui->cmdStartGraph->setVisible(false);
        ui->cmdStopGraph->setVisible(false);
        ui->chkRawData->setVisible(false);
        ui->cmbRemap->setVisible(false);
        ui->cmbSigns->setVisible(false);
        ui->stackedWidget->setCurrentIndex(3);

        ui->cmdSaveNVM->setVisible(true);
        ui->grbSettings->setTitle("Nano 33 BLE");

        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        fwdiscovered=true;
    } else if (hard == "BNO055") {
        ui->cmdStartGraph->setVisible(true);
        ui->cmdStopGraph->setVisible(true);
        ui->cmbRemap->setVisible(true);
        ui->cmbSigns->setVisible(true);
        ui->chkRawData->setVisible(true);
        ui->cmdSaveNVM->setVisible(false);
        ui->stackedWidget->setCurrentIndex(2);
        ui->grbSettings->setTitle("BNO055");

        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        fwdiscovered=true;
    } else {

    }
}

/* - Checks if the boards firmware was actually found.
 * This is called 1sec after
 */

void MainWindow::connectTimeout()
{
    if(!fwdiscovered && serialcon->isOpen()) {
        QMessageBox::information(this,"Error", "No valid board detected\nPlease check COM port or flash proper code");
        serialDisconnect();
    }
}

/* Called if waiting for an ack or nak doesn't ever arrive (~1 second)
 *  Also called on a nak received from a bad CRC on headtracker
 *   Try re-sending data
 */

void MainWindow::comTimeout()
{
    comtimeout.stop();

    // If too many faults, disconnect.
    if(jsonfaults > MAX_TX_FAULTS) {
        addToLog("\r\nERROR: Critical - " + QString(MAX_TX_FAULTS)+ " transmission faults, disconnecting\r\n");
        serialDisconnect();

    } else {
        // Pause a bit, give time for device to catch up
        Sleep(TX_FAULT_PAUSE);

        // Resend last JSON
        sendSerialData(lastjson);
        addToLog("ERROR: CRC Fault - Re-sending data (" +  lastjson + ")\n");
    }

    // Increment fault counter
    jsonfaults++;
}

/* Decide what to do with incoming JSON packet
 * v1.0 Possible Incoming JSON Commands
 *      "Settings"  Tracker Settings Send
 *      "Data" Live Data for Info / Calibration
 *      "FW"  Firmware Version + Hardware
 */

void MainWindow::parseIncomingJSON(const QVariantMap &map)
{
    // Settings from the Tracker Sent, save them and update the UI
    if(map["Cmd"].toString() == "Settings") {        
        trkset.setAllData(map);
        updateToUI();

    // Data sent, Update the graph / servo sliders / calibration
    } else if (map["Cmd"].toString() == "Data") {
        // Add all the data to the settings
        trkset.setLiveDataMap(map);

        // Remind user to calibrate
        if(map["magcal"].toBool() == false && calmsgshowed == false && fwdiscovered) {
            msgbox.setText("Calibration has not been performed.\nPlease calibrate or load from a saved file");
            msgbox.setWindowTitle("Calibrate");
            msgbox.show();
            ui->statusbar->showMessage(tr("Not calibrated"),10000);
            calmsgshowed = true;
        }

        // Set BLE Address on GUI
        ui->lblBLEAddress->setText("Addr: " + trkset.liveData("btaddr").toString());

    // Firmware Hardware and Version
    } else if (map["Cmd"].toString() == "FW") {
        fwDiscovered(map["Vers"].toString(), map["Hard"].toString());
    }
}

/* parseIncomingHT()
 *      Read older head tracker code
 */

void MainWindow::parseIncomingHT(QString cmd)
{
    static QString vers;
    static QString hard;

    // CRC ERROR
    if(cmd.left(7) == "$CRCERR") {
        addToLog("Headtracker CRC Error!\n");
        ui->statusbar->showMessage("CRC Error : Error Setting Values, Retrying",2000);
        updatesettingstmr.start(1000); // Resend
    }

    // CRC OK
    else if(cmd.left(6) == "$CRCOK") {
        ui->statusbar->showMessage("Values Set On Headtracker",2000);
        sentToHT = true;
        ui->cmdStore->setEnabled(false);
    }

    // Calibration Saved
    else if(cmd.left(7) == "$CALSAV") {
        ui->statusbar->showMessage("Calibration Saved", 2000);
    }

    // Graph Data
    else if(cmd.left(2) == "$G") {
        cmd = cmd.mid(2).simplified();
        QStringList rtd = cmd.split(',');
        if(rtd.length() == 10) {
            QVariantMap vm;
            if(rawmode) {
                vm["tilt"] = rtd.at(0);
                vm["roll"] = rtd.at(1);
                vm["pan"] = rtd.at(2);
            } else {
                vm["tiltoff"] = rtd.at(0);
                vm["rolloff"] = rtd.at(1);
                vm["panoff"] = rtd.at(2);
            }
            vm["panout"] = rtd.at(3);
            vm["tiltout"] = rtd.at(4);
            vm["rollout"] = rtd.at(5);
            vm["syscal"] = rtd.at(6);
            vm["gyrocal"] = rtd.at(7);
            vm["accelcal"] = rtd.at(8);
            vm["magcal"] = rtd.at(9);
            trkset.setLiveDataMap(vm);
            graphing = true;
            bnoCalibratorDialog->setCalibration(vm["syscal"].toInt(),
                                             vm["magcal"].toInt(),
                                             vm["gyrocal"].toInt(),
                                             vm["accelcal"].toInt());
        }
    }
    // Setting Data
    else if(cmd.left(5) == "$SET$") {
        QStringList setd = cmd.right(cmd.length()-5).split(',',Qt::KeepEmptyParts);
        if(setd.length() == trkset.count()) {
            trkset.setLPTiltRoll(setd.at(0).toFloat());
            trkset.setLPPan(setd.at(1).toFloat());
            trkset.setGyroWeightTiltRoll(setd.at(2).toFloat());
            trkset.setGyroWeightPan(setd.at(3).toFloat());
            trkset.setTlt_gain(setd.at(4).toFloat() /10);
            trkset.setPan_gain(setd.at(5).toFloat()/10);
            trkset.setRll_gain(setd.at(6).toFloat()/10);
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
    } else if(cmd.left(5) == "$VERS") {
        vers = cmd.mid(5);
        if(!hard.isEmpty())
            fwDiscovered(vers,hard);
    } else if(cmd.left(5) == "$HARD") {
        hard = cmd.mid(5);
        if(!vers.isEmpty())
                fwDiscovered(vers,hard);
    }
}


/* sendSerialData()
 *      Send Raw Data To The Serial Port
 */

void MainWindow::sendSerialData(QByteArray data)
{
    if(data.isEmpty() || !serialcon->isOpen())
        return;    

    ui->txled->setState(true);
    txledtimer.start();
    serialcon->write(data);

    // Skip nuisance I'm here message
    if(QString(data).contains("{\"Cmd\":\"IH\"}"))
        return;

    addToLog("GUI: " + data + "\n");
}

/* sendSerialJSON()
 *      Sends a JSON Packet
 */

void MainWindow::sendSerialJSON(QString command, QVariantMap map)
{
    // Don't send any new data until last has been received successfully
    map.remove("Hard");
    map.remove("Vers");
    QJsonObject jobj = QJsonObject::fromVariantMap(map);
    jobj["Cmd"] = command;
    QJsonDocument jdoc(jobj);
    QString json = QJsonDocument(jdoc).toJson(QJsonDocument::Compact);

    // Calculate the CRC Checksum
    uint16_t CRC = escapeCRC(uCRC16Lib::calculate(json.toUtf8().data(),json.length()));

    qDebug() << "CRC" << CRC << "Len" << json.length();
    lastjson = (char)0x02 + json.toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + (char)0x03;

    // If there is data that didn't make it there yet push this data to the queue
    // to be sent later
    if(jsonfaults != 0) {
        jsonqueue.enqueue(lastjson);
        return;
    }

    // Send the data
    sendSerialData(lastjson);

    // Set as faulted until ACK returned
    jsonfaults = 1;

    // Reset Ack Timer
    imheretimout.stop();
    imheretimout.start(IMHERETIME);
}

void MainWindow::requestTimer()
{
    // Start by blasting out all the startup commands for all tracker variants
    // will figure out which one it is on response

    // Request in HT Mode
    sendSerialData("$VERS");
    sendSerialData("$HARD");
    sendSerialData("$GSET");

    // Request in JSON Mode
    sendSerialJSON("FW"); // Get the firmware
    sendSerialJSON("GetSet"); // Get the Settings
    sendSerialJSON("IH"); // Start Data Transfer right away (Im here)

    // Timer to make sure it a response within 1 sec after sending this data
    connectTimer.stop();
    connectTimer.setInterval(1000);
    connectTimer.start();
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
    serialDebug->setPlainText(logd);

    // Scroll to bottom
    serialDebug->verticalScrollBar()->setValue(serialDebug->verticalScrollBar()->maximum());
}

uint16_t MainWindow::escapeCRC(uint16_t crc)
{
    // Characters to escape out
    uint8_t crclow = crc & 0xFF;
    uint8_t crchigh = (crc >> 8) & 0xFF;
    if(crclow == 0x00 ||
       crclow == 0x02 ||
       crclow == 0x03 ||
       crclow == 0x06 ||
       crclow == 0x15)
        crclow ^= 0xFF; //?? why not..
    if(crchigh == 0x00 ||
       crchigh == 0x02 ||
       crchigh == 0x03 ||
       crchigh == 0x06 ||
       crchigh == 0x15)
        crchigh ^= 0xFF; //?? why not..
    return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

uint16_t MainWindow::escapeCRCHT(uint16_t crc)
{
    // Characters to escape out
    uint8_t crclow = crc & 0xFF;
    uint8_t crchigh = (crc >> 8) & 0xFF;
    if(crclow == 0x00 ||
       crclow == 0x24)
        crclow ^= 0xFF; //?? why not..
    if(crchigh == 0x00 ||
       crchigh == 0x24)
        crchigh ^= 0xFF; //?? why not..
    return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->type() == QKeyEvent::KeyPress) {
        if((event->modifiers() & Qt::ControlModifier) &&
           (event->key() & Qt::Key_D)) {
            // Ctrl - D Pressed
            diagnostic->show();
            diagnostic->activateWindow();
            diagnostic->raise();
        }
    }
}

// Finds available serial ports
void MainWindow::findSerialPorts()
{
    ui->cmbPort->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port,ports) {
        ui->cmbPort->addItem(port.portName(),port.serialNumber());
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


    ui->spnLPTiltRoll->setValue(trkset.lpTiltRoll());
    ui->spnLPPan->setValue(trkset.lpPan());

    ui->chkpanrev->setChecked(trkset.isPanReversed());
    ui->chkrllrev->setChecked(trkset.isRollReversed());
    ui->chktltrev->setChecked(trkset.isTiltReversed());
    ui->chkInvertedPPM->setChecked(trkset.invertedPpmOut());
    ui->chkInvertedPPMIn->setChecked(trkset.invertedPpmIn());
    ui->chkResetCenterWave->setChecked(trkset.resetOnWave());

    // Prevents a loop
    ui->cmbpanchn->blockSignals(true);
    ui->cmbrllchn->blockSignals(true);
    ui->cmbtiltchn->blockSignals(true);
    ui->cmbRemap->blockSignals(true);
    ui->cmbPpmOutPin->blockSignals(true);
    ui->cmbPpmInPin->blockSignals(true);
    ui->cmbButtonPin->blockSignals(true);
    ui->cmbBtMode->blockSignals(true);
    ui->cmbOrientation->blockSignals(true);
    ui->cmbResetOnPPM->blockSignals(true);
    ui->spnLPPan->blockSignals(true);
    ui->spnLPTiltRoll->blockSignals(true);
    ui->til_gain->blockSignals(true);
    ui->rll_gain->blockSignals(true);
    ui->pan_gain->blockSignals(true);

    ui->cmbpanchn->setCurrentIndex(trkset.panCh()-1);
    ui->cmbrllchn->setCurrentIndex(trkset.rollCh()-1);
    ui->cmbtiltchn->setCurrentIndex(trkset.tiltCh()-1);
    ui->cmbRemap->setCurrentIndex(ui->cmbRemap->findData(trkset.axisRemap()));
    ui->cmbSigns->setCurrentIndex(trkset.axisSign());
    ui->cmbBtMode->setCurrentIndex(trkset.blueToothMode());    
    ui->cmbOrientation->setCurrentIndex(trkset.orientation());
    ui->til_gain->setValue(trkset.Tlt_gain()*10);
    ui->pan_gain->setValue(trkset.Pan_gain()*10);
    ui->rll_gain->setValue(trkset.Rll_gain()*10);


    int ppout_index = trkset.ppmOutPin()-1;
    int ppin_index = trkset.ppmInPin()-1;
    int but_index = trkset.buttonPin()-1;
    int resppm_index = trkset.resetCntPPM();
    ui->cmbPpmOutPin->setCurrentIndex(ppout_index < 1 ? 0 : ppout_index);
    ui->cmbPpmInPin->setCurrentIndex(ppin_index < 1 ? 0 : ppin_index);
    ui->cmbButtonPin->setCurrentIndex(but_index < 1 ? 0 : but_index);
    ui->cmbResetOnPPM->setCurrentIndex(resppm_index < 0 ? 0: resppm_index);

    ui->cmbpanchn->blockSignals(false);
    ui->cmbrllchn->blockSignals(false);
    ui->cmbtiltchn->blockSignals(false);
    ui->cmbRemap->blockSignals(false);
    ui->cmbPpmOutPin->blockSignals(false);
    ui->cmbPpmInPin->blockSignals(false);
    ui->cmbButtonPin->blockSignals(false);
    ui->cmbBtMode->blockSignals(false);
    ui->cmbOrientation->blockSignals(false);
    ui->cmbResetOnPPM->blockSignals(false);
    ui->spnLPPan->blockSignals(false);
    ui->spnLPTiltRoll->blockSignals(false);
    ui->til_gain->blockSignals(false);
    ui->rll_gain->blockSignals(false);
    ui->pan_gain->blockSignals(false);


    savedToNVM = true;
    sentToHT = true;
    ui->cmdStore->setEnabled(false);
}

// Update the Settings Class from the UI Data
void MainWindow::updateFromUI()
{
    if(!serialcon->isOpen()) // Don't update anything if not connected
        return;
    trkset.setPan_cnt(ui->servoPan->centerValue());
    trkset.setPan_min(ui->servoPan->minimumValue());
    trkset.setPan_max(ui->servoPan->maximumValue());
    trkset.setPan_gain(static_cast<float>(ui->pan_gain->value())/10.0f);

    trkset.setTlt_cnt(ui->servoTilt->centerValue());
    trkset.setTlt_min(ui->servoTilt->minimumValue());
    trkset.setTlt_max(ui->servoTilt->maximumValue());
    trkset.setTlt_gain(static_cast<float>(ui->til_gain->value())/10.0f);

    trkset.setRll_cnt(ui->servoRoll->centerValue());
    trkset.setRll_min(ui->servoRoll->minimumValue());
    trkset.setRll_max(ui->servoRoll->maximumValue());
    trkset.setRll_gain(static_cast<float>(ui->rll_gain->value())/10.0f);

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

    // Shift the index of the disabled choice to -1 in settings
    int ppout_index = ui->cmbPpmOutPin->currentIndex()+1;
    ppout_index = ppout_index==1?-1:ppout_index;
    int ppin_index = ui->cmbPpmInPin->currentIndex()+1;
    ppin_index = ppin_index==1?-1:ppin_index;
    int but_index = ui->cmbButtonPin->currentIndex()+1;
    but_index = but_index==1?-1:but_index;

    // Check for pin duplicates
    if((but_index   > 0 && (but_index == ppin_index || but_index == ppout_index)) ||
       (ppin_index > 0 && (ppin_index == but_index || ppin_index == ppout_index)) ||
       (ppout_index > 0 && (ppout_index == but_index || ppout_index == ppin_index))) {
        QMessageBox::information(this,"Error", "Cannot pick dulplicate pins");

        // Reset gui to old values
        ppout_index = trkset.ppmOutPin()-1;
        ppin_index = trkset.ppmInPin()-1;
        but_index = trkset.buttonPin()-1;
        ui->cmbPpmOutPin->setCurrentIndex(ppout_index < 1 ? 0 : ppout_index);
        ui->cmbPpmInPin->setCurrentIndex(ppin_index < 1 ? 0 : ppin_index);
        ui->cmbButtonPin->setCurrentIndex(but_index < 1 ? 0 : but_index);
    } else {
        trkset.setPpmOutPin(ppout_index);
        trkset.setPpmInPin(ppin_index);
        trkset.setButtonPin(but_index);
    }

    int rstppm_index = ui->cmbResetOnPPM->currentIndex();
    trkset.setResetCntPPM(rstppm_index==0?-1:rstppm_index);

    trkset.setBlueToothMode(ui->cmbBtMode->currentIndex());
    trkset.setOrientation(ui->cmbOrientation->currentIndex());

    trkset.setInvertedPpmOut(ui->chkInvertedPPM->isChecked());
    trkset.setInvertedPpmIn(ui->chkInvertedPPMIn->isChecked());
    trkset.setResetOnWave(ui->chkResetCenterWave->isChecked());

    savedToNVM = false; // Indicate values have not been save to NVM
    sentToHT = false; // Indicate changes haven't been sent to HT
    ui->cmdStore->setEnabled(true);

    updatesettingstmr.start(500);
}

// Data ready to be read from the serial port
void MainWindow::serialReadReady()
{
    // Read all serial data
    bool scroll = false;
    QByteArray sd = serialcon->readAll();
    serialData.append(sd);

    int slider = serialDebug->verticalScrollBar()->value();
    if(slider == serialDebug->verticalScrollBar()->maximum())
        scroll = true; 

    // Scroll to bottom
    if(scroll)
        serialDebug->verticalScrollBar()->setValue(serialDebug->verticalScrollBar()->maximum());
    else
        serialDebug->verticalScrollBar()->setValue(slider);

    parseSerialData();

    ui->rxled->setState(true);
    rxledtimer.start();
}

void MainWindow::manualSend()
{
    sendSerialData(ui->txtCommand->text().toLatin1());
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

void MainWindow::startGraph()
{
    if(!serialcon->isOpen())
        return;
    sendSerialData("$PLST");
    graphing = true;
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
}

void MainWindow::stopGraph()
{
    if(!serialcon->isOpen())
        return;
    sendSerialData("$PLEN");
    graphing = false;
    ui->servoPan->setShowActualPosition(false);
    ui->servoTilt->setShowActualPosition(false);
    ui->servoRoll->setShowActualPosition(false);
}

// Send All Settings to the Controller

void MainWindow::storeSettings()
{
    if(!serialcon->isOpen())
        return;

    // Send Data to the NANO33BLE
    if(trkset.hardware() == "NANO33BLE") {
        QVariantMap d2s = trkset.changedData();
        // Remove useless items
        d2s.remove("axisremap");
        d2s.remove("axissign");
        d2s.remove("Hard");
        d2s.remove("Vers");
        // If no changes, return
        if(d2s.count() == 0)
            return;

        // Send Changed Data
        sendSerialJSON("Setttings", d2s);
        // Set the data is now matched on the device
        trkset.setDataMatched();
        // Flag for exit, has data been sent
        sentToHT = true;
        ui->cmdStore->setEnabled(false);

    // Send data to the BNO055
    } else if(trkset.hardware() == "BNO055") {
        // Disable Graphing output, all the extra data was causing issues
        // on update
        if(graphing) {
            sendSerialData("$PLEN");
        }

        updatesettingstmr.stop();
        QStringList lst;
        lst.append(QString::number(trkset.lpTiltRoll()));
        lst.append(QString::number(trkset.lpPan()));
        lst.append(QString::number(trkset.gyroWeightTiltRoll()));
        lst.append(QString::number(trkset.gyroWeightPan()));
        lst.append(QString::number(trkset.Tlt_gain()*10));
        lst.append(QString::number(trkset.Pan_gain()*10));
        lst.append(QString::number(trkset.Rll_gain()*10));
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
        uint16_t CRC = escapeCRCHT(uCRC16Lib::calculate(data.toUtf8().data(),data.length()));

        // Append Data in a Byte Array
        QByteArray bd = "$" + QString(data).toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + "HE";

        sendSerialData(bd);

        // Re-Enable Graphing if required
        if(graphing) {
            sendSerialData("$PLST");
        }
    }

    diagnostic->update();
    ui->statusbar->showMessage(tr("Settings Sent"),2000);
}

// Automatic Send Changes.
void MainWindow::updateSettings()
{
    storeSettings();
}

void MainWindow::resetCenter()
{
    if(trkset.hardware() == "NANO33BLE")
        sendSerialJSON("RstCnt");
    else if(trkset.hardware() == "BNO055")
        sendSerialData("$RST");
}

void MainWindow::setDataMode(bool rm)
{
    // Change mode to show offset vs raw unfiltered data
    if(rm)
        sendSerialData("$GRAW ");
    else
        sendSerialData("$GOFF ");

    rawmode = rm;
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
    QString filename;
    QFileDialog savefd(this);
    savefd.setWindowTitle("Save Settigns");
    savefd.setNameFilter("Config Files (*.ini)");
    if(savefd.exec()) {
        if(savefd.selectedFiles().count()) {
            filename = savefd.selectedFiles().at(0);
            QSettings settings(filename,QSettings::IniFormat);
            trkset.storeSettings(&settings);
        }
    }
}

void MainWindow::loadSettings()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this, "Info","Please connect before restoring a saved file");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this,tr("Open File"),QString(),"Config Files (*.ini)");
    if(!filename.isEmpty()) {
        QSettings settings(filename,QSettings::IniFormat);
        trkset.loadSettings(&settings);
        updateToUI();
        updateSettings();
    }
}

bool MainWindow::checkSaved()
{
    bool close=true;
    if(!sentToHT) {
        QMessageBox::StandardButton rval = QMessageBox::question(this,"Changes not sent","Are you sure you want to disconnect?\n"\
                              "Changes haven't been sent to the headtracker\nClick \"Send Changes\" first",QMessageBox::Yes|QMessageBox::No);
        if(rval != QMessageBox::Yes)
            close = false;
    } else if(!savedToNVM && trkset.hardware() == "NANO33BLE") { // Ignore on BNO055
        QMessageBox::StandardButton rval = QMessageBox::question(this,"Changes not saved on tracker","Are you sure you want to disconnet?\n"\
                              "Changes haven't been permanently stored on headtracker\nClick \"Save to NVM\" first",QMessageBox::Yes|QMessageBox::No);
        if(rval != QMessageBox::Yes)
            close = false;
    }
    return close;
}

void MainWindow::uploadFirmwareClick()
{
    if(serialcon->isOpen()) {
        QMessageBox::information(this,"Cannot Upload", "Disconnect before uploading a new firmware");
    } else {
        firmwareUploader->show();
        firmwareUploader->activateWindow();
        firmwareUploader->raise();
    }
}

// Start the various calibration dialogs

void MainWindow::startCalibration()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this,"Not Connected", "Connect before trying to calibrate");
        return;
    }

    if(trkset.hardware() == "NANO33BLE") {
        bleCalibratorDialog->show();

    } else if (trkset.hardware() == "BNO055") {
        // Start calibration, start graphing.
        sendSerialData("$STO");
        startGraph();
        bnoCalibratorDialog->startCalibration();
        bnoCalibratorDialog->show();
    }
}

// Nano33BLE - Let hardware know were here (I'm Here)

void MainWindow::ihTimeout()
{
    if(serialcon->isOpen() && trkset.hardware() == "NANO33BLE") {
        sendSerialJSON("IH");
    }
}

// BNO055 saves to EEPROM on receive
// NANO 33 BLE user must click the button so there aren't as many write cycles
// wearing out the flash.

void MainWindow::saveToNVM()
{
    if(serialcon->isOpen()) {
        if(trkset.hardware() == "NANO33BLE") {
            sendSerialJSON("Flash");
            savedToNVM = true;
        }
    }
}

void MainWindow::showDiagsClicked()
{
    diagnostic->show();
    diagnostic->activateWindow();
    diagnostic->raise();
}

void MainWindow::showSerialDiagClicked()
{
    serialDebug->show();
    serialDebug->activateWindow();
    serialDebug->raise();
}

void MainWindow::closeEvent(QCloseEvent *event)
{    
    bool close=checkSaved();

    if(close) {
        QCoreApplication::quit();
        event->accept();
    } else
        event->ignore();
}

