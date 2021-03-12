
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servominmax.h"
#include "ucrc16lib.h"

#define DEBUG_HT

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    waitingOnParameters = false;

    // Start the board interface
    nano33ble = new BoardNano33BLE(&trkset);
    bno055 = new BoardBNO055(&trkset);

    // Add it to the list of available boards
    boards.append(nano33ble);
    boards.append(bno055);

    // Once correct board is discovered this will be set to one of the above boards
    currentboard = nullptr;

    setWindowTitle(windowTitle() + " " + version);

    // Serial Connection
    serialcon = new QSerialPort;

    // Diagnostic Display + Serial Debug
    diagnostic = new DiagnosticDisplay(&trkset);
    serialDebug = new QPlainTextEdit();
    serialDebug->setWindowTitle("Serial Information");
    serialDebug->resize(600,300);

#ifdef DEBUG_HT
    serialDebug->show();
    diagnostic->show();
#endif

    // Hide these buttons until connected
    ui->cmdStartGraph->setVisible(false);
    ui->cmdStopGraph->setVisible(false);
    ui->chkRawData->setVisible(false);

    // Firmware loader loader dialog
    firmwareUploader = new Firmware;

    // Get list of available ports
    findSerialPorts();

    // Use system default fixed witdh font
    QFont serifFont("Times", 10, QFont::Bold);
    serialDebug->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // Update default settings to UI
    updateToUI();

    // Board Connections, connects all boards signals to same end points
    foreach(BoardType *brd, boards) {
        connect(brd,SIGNAL(paramSendStart()), this, SLOT(paramSendStart()));
        connect(brd,SIGNAL(paramSendFailure(int)), this, SLOT(paramSendFailure(int)));
        connect(brd,SIGNAL(paramReceiveStart()), this, SLOT(paramReceiveStart()));
        connect(brd,SIGNAL(paramReceiveComplete()), this, SLOT(paramReceiveComplete()));
        connect(brd,SIGNAL(paramReceiveFailure(int)), this, SLOT(paramReceiveFailure(int)));
        connect(brd,SIGNAL(calibrationSuccess()), this, SLOT(calibrationSuccess()));
        connect(brd,SIGNAL(calibrationFailure()), this, SLOT(calibrationFailure()));
        connect(brd,SIGNAL(serialTxReady()), this, SLOT(serialTxReady()));
        connect(brd,SIGNAL(addToLog(QString)),this,SLOT(addToLog(QString)));
        connect(brd,SIGNAL(needsCalibration()),this,SLOT(needsCalibration()));
        connect(brd,SIGNAL(boardDiscovered(BoardType *)),this,SLOT(boardDiscovered(BoardType *)));
        connect(brd,SIGNAL(statusMessage(QString,int)),this,SLOT(statusMessage(QString,int)));
    }

    // Serial data ready
    connect(serialcon,SIGNAL(readyRead()),this,SLOT(serialReadReady()));
    connect(serialcon, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),this,SLOT(serialError(QSerialPort::SerialPortError)));

    // Buttons
    connect(ui->cmdConnect,SIGNAL(clicked()),this,SLOT(serialConnect()));
    connect(ui->cmdDisconnect,SIGNAL(clicked()),this,SLOT(serialDisconnect()));    
    connect(ui->cmdStore,SIGNAL(clicked()),this,SLOT(storeToRAM()));
    connect(ui->cmdSend,SIGNAL(clicked()),this,SLOT(manualSend()));
    //connect(ui->cmdStartGraph,SIGNAL(clicked()),this,SLOT(startGraph()));
    //connect(ui->cmdStopGraph,SIGNAL(clicked()),this,SLOT(stopGraph()));
    connect(ui->cmdResetCenter,SIGNAL(clicked()),this, SLOT(resetCenter()));
    connect(ui->cmdCalibrate,SIGNAL(clicked()),this, SLOT(startCalibration()));
    connect(ui->cmdSaveNVM,SIGNAL(clicked()),this,SLOT(storeToNVM()));
    //***
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // Check Boxes
    connect(ui->chkpanrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkrllrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chktltrev,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkInvertedPPM,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkInvertedPPMIn,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    connect(ui->chkResetCenterWave,SIGNAL(clicked(bool)),this,SLOT(updateFromUI()));
    //connect(ui->chkRawData,SIGNAL(clicked(bool)),this,SLOT(setDataMode(bool)));

    // Spin Boxes
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
    connect(&rxledtimer,SIGNAL(timeout()),this,SLOT(rxledtimeout()));
    rxledtimer.setInterval(100);
    connect(&txledtimer,SIGNAL(timeout()),this,SLOT(txledtimeout()));
    txledtimer.setInterval(100);
    connect(&connectTimer,SIGNAL(timeout()),this,SLOT(connectTimeout()));
    connectTimer.setSingleShot(true);
    connect(&requestTimer,SIGNAL(timeout()),this,SLOT(requestTimeout()));
    requestTimer.setSingleShot(true);
    connect(&saveToRAMTimer,SIGNAL(timeout()),this,SLOT(saveToRAMTimeout()));
    saveToRAMTimer.setSingleShot(true);
    connect(&requestParamsTimer,SIGNAL(timeout()),this,SLOT(requestParamsTimeout()));
    requestParamsTimer.setSingleShot(true);

    // Called to initalize GUI state to disconnected
    serialDisconnect();
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete nano33ble;
    delete firmwareUploader;
    delete serialDebug;
    delete ui;
}

// Connects to the serial port

void MainWindow::serialConnect()
{
    QString port = ui->cmbPort->currentText();
    if(port.isEmpty())
        return;

    // If open close it first
    if(serialcon->isOpen())
        serialDisconnect();

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
    statusMessage(tr("Connected to ") + serialcon->portName());
    ui->stackedWidget->setCurrentIndex(1);

    serialcon->setDataTerminalReady(true);

    // Begin the request to discover boards
    addToLog("Waiting for board to boot...\n");

    requestTimer.stop();
    requestTimer.start(4000);
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

        // Notify board connection is disconnected
        foreach(BoardType *brd, boards) {
            brd->_disconnected();
            brd->allowAccess(false);
        }

        serialcon->flush();
        serialcon->close();
    }
    statusMessage(tr("Disconnected"));
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
    ui->servoPan->setShowActualPosition(false);
    ui->servoTilt->setShowActualPosition(false);
    ui->servoRoll->setShowActualPosition(false);

    currentboard = nullptr;
    boardRequestIndex=0;
    connectTimer.stop();
    requestTimer.stop();

    // Notify all boards we have disconnected
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

/* - Checks if the boards firmware was actually found.
 *   This is called 1sec after a request is sent for the board version
 */

void MainWindow::connectTimeout()
{
    if(currentboard != nullptr && serialcon->isOpen()) {
        QMessageBox::information(this,"Error", "No valid board detected\nPlease check COM port or flash proper code");
        serialDisconnect();
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
    // Don't update GUI if haven't got data from the device yet
    if(waitingOnParameters)
        return;

    ui->servoTilt->setCenter(trkset.Tlt_cnt());
    ui->servoTilt->setMaximum(trkset.Tlt_max());
    ui->servoTilt->setMinimum(trkset.Tlt_min());

    ui->servoPan->setCenter(trkset.Pan_cnt());
    ui->servoPan->setMaximum(trkset.Pan_max());
    ui->servoPan->setMinimum(trkset.Pan_min());

    ui->servoRoll->setCenter(trkset.Rll_cnt());
    ui->servoRoll->setMaximum(trkset.Rll_max());
    ui->servoRoll->setMinimum(trkset.Rll_min());


    ui->chkpanrev->setChecked(trkset.isPanReversed());
    ui->chkrllrev->setChecked(trkset.isRollReversed());
    ui->chktltrev->setChecked(trkset.isTiltReversed());
    ui->chkInvertedPPM->setChecked(trkset.invertedPpmOut());
    ui->chkInvertedPPMIn->setChecked(trkset.invertedPpmIn());
    ui->chkResetCenterWave->setChecked(trkset.resetOnWave());

    // Prevents signals from these items causing another update
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

    ui->spnLPTiltRoll->setValue(trkset.lpTiltRoll());
    ui->spnLPPan->setValue(trkset.lpPan());
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

    ui->cmdStore->setEnabled(true);
    ui->cmdSaveNVM->setEnabled(true);

    // Use timer to prevent too many writes while drags, etc.. happen
    saveToRAMTimer.start(500);
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

        // Send complete lines to
        foreach(BoardType *brd, boards) {
            brd->_dataIn(data);
        }

        serialData = serialData.right(serialData.length()-nlindex-2);
    }

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

    // Good enough spot to update it...
    ui->lblBLEAddress->setText("Address: " + trkset.blueToothAddress());
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
    QString filename = QFileDialog::getSaveFileName(this,"Save Settings",QString(), "Config Files (*.ini)");
    if(!filename.isEmpty()) {
        QSettings settings(filename,QSettings::IniFormat);
        trkset.storeSettings(&settings);
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
        storeToRAM();
    }
}

bool MainWindow::checkSaved()
{
    foreach(BoardType *brd, boards) {
        if(!brd->isAccessAllowed())
            continue;
        if(!brd->_isBoardSavedToRAM()) {
            QMessageBox::StandardButton rval = QMessageBox::question(this,"Changes not sent","Are you sure you want to disconnect?\n"\
                                  "Changes haven't been sent to the headtracker\nClick \"Send Changes\" first",QMessageBox::Yes|QMessageBox::No);
            if(rval != QMessageBox::Yes)
                return false;

        } else if (!brd->_isBoardSavedToNVM()) {
            QMessageBox::StandardButton rval = QMessageBox::question(this,"Changes not saved on tracker","Are you sure you want to disconnet?\n"\
                                  "Changes haven't been permanently stored on headtracker\nClick \"Save to NVM\" first",QMessageBox::Yes|QMessageBox::No);
            if(rval != QMessageBox::Yes)
                return false;
        }
    }
    return true;
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

/* requestTimeout()
 *      This timeout is called after waiting for the board to boot
 * it requests the hardware and version from the board, gives each board
 * 250ms to respond before trying the next one. Sets the allowAccess so
 * other boards won't respond on the serial line at the same time.
 */

void MainWindow::requestTimeout()
{
    // Was a board discovered, if so just quit
    if(currentboard != nullptr)
        return;

    // Otherwise increment to the next board and try again
    if(boardRequestIndex == boards.length()) {
        msgbox.setText("Was unable to determine the board type");
        msgbox.setWindowTitle("Error");
        msgbox.show();
        statusMessage("Board discovery failed");
        serialDisconnect();
        return;
    }

    // Prevent last board class from interfering
    if(boardRequestIndex > 0)
        boards[boardRequestIndex-1]->allowAccess(false);

    // Request hardware information from the new board
    addToLog("Trying to connect to " + boards[boardRequestIndex]->boardName() + "\n");
    boards[boardRequestIndex]->allowAccess(true);
    boards[boardRequestIndex]->requestHardware();
    requestTimer.start(250);

    // Move to next board
    boardRequestIndex++;
}

void MainWindow::saveToRAMTimeout()
{
    // Request hardware from all board types
    foreach(BoardType *brd, boards) {
        brd->_saveToRAM();
        ui->cmdStore->setEnabled(false);
    }
}

void MainWindow::requestParamsTimeout()
{
    waitingOnParameters=true;
    // Request hardware from all board types
    foreach(BoardType *brd, boards) {
        brd->_requestParameters();
    }
}

// Start the various calibration dialogs
void MainWindow::startCalibration()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this,"Not Connected", "Connect before trying to calibrate");
        return;
    }

    foreach(BoardType *brd, boards) {
        brd->_startCalibration();
    }
}

void MainWindow::storeToNVM()
{
    foreach(BoardType *brd, boards) {
        brd->_saveToNVM();
    }
}

void MainWindow::storeToRAM()
{
    foreach(BoardType *brd, boards) {
        brd->_saveToRAM();
    }
}

void MainWindow::resetCenter()
{
    foreach(BoardType *brd, boards) {
        brd->_resetCenter();
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

void MainWindow::paramSendStart()
{
    statusMessage("Starting parameter send");
}

void MainWindow::paramSendComplete()
{
    statusMessage("Parameter(s) saved");
    ui->cmdStore->setEnabled(false);
}

void MainWindow::paramSendFailure(int)
{
    msgbox.setText("Unable to upload the parameter(s)");
    msgbox.setWindowTitle("Error");
    msgbox.show();
    statusMessage("Parameters Send Failure");
    serialDisconnect();
}

void MainWindow::paramReceiveStart()
{
    statusMessage("Parameters Request Started",5000);
}

void MainWindow::paramReceiveComplete()
{
    statusMessage("Parameters Request Started",5000);
    waitingOnParameters = false;
    updateToUI();
}

void MainWindow::paramReceiveFailure(int)
{
    msgbox.setText("Unable to receive the parameters");
    msgbox.setWindowTitle("Error");
    msgbox.show();
    statusMessage("Parameter Received Failure");
    serialDisconnect();
}

void MainWindow::calibrationSuccess()
{
    statusMessage("Calibration Success",5000);
}

void MainWindow::calibrationFailure()
{
    statusMessage("Calibration Failure",5000);
}

void MainWindow::serialTxReady()
{
    foreach(BoardType *brd, boards) {
        sendSerialData(brd->_dataout());
    }
}

void MainWindow::needsCalibration()
{
    msgbox.setText("Calibration has not been performed.\nPlease calibrate or restore from a saved file");
    msgbox.setWindowTitle("Calibrate");
    msgbox.show();
}

void MainWindow::boardDiscovered(BoardType *brd)
{
    // Board discovered, save it
    currentboard = brd;

    // Stack widget changes to hide some info depending on board
    if(brd->boardName() == "NANO33BLE") {
        addToLog("Connect to a " + brd->boardName() + "\n");
        ui->cmdStartGraph->setVisible(false);
        ui->cmdStopGraph->setVisible(false);
        ui->chkRawData->setVisible(false);
        ui->cmbRemap->setVisible(false);
        ui->cmbSigns->setVisible(false);
        ui->cmdSaveNVM->setVisible(true);
        ui->grbSettings->setTitle("Nano 33 BLE");
        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(3);

    } else if (brd->boardName() == "BNO055") {
        addToLog("Connect to a " + brd->boardName() + "\n");
        ui->cmdStartGraph->setVisible(true);
        ui->cmdStopGraph->setVisible(true);
        ui->cmbRemap->setVisible(true);
        ui->cmbSigns->setVisible(true);
        ui->chkRawData->setVisible(true);
        ui->cmdSaveNVM->setVisible(false);
        ui->grbSettings->setTitle("BNO055");
        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(2);

    } else if (brd->boardName() == "NANO33REMOTE") {
        addToLog("Connect to a " + brd->boardName() + "\n");

    } else {
        msgbox.setText("Unknown board type");
        msgbox.setWindowTitle("Error");
        msgbox.show();
        statusMessage("Unknown board type");
        serialDisconnect();
    }

    // Request the parameters from the device now
    requestParamsTimer.start(50);
}

void MainWindow::statusMessage(QString str, int timeout)
{
    ui->statusbar->showMessage(str,timeout);
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

