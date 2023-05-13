
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servominmax.h"
#include "ucrc16lib.h"


//#define DEBUG_HT

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    waitingOnParameters = false;
    msgbox = new QMessageBox(this);

    // Pin Viewer
    imageViewer = new ImageViewer(this);
    imageViewer->setWindowFlag(Qt::Window);
    imageViewer->setImage(QPixmap(":/Icons/images/Pinout.png").toImage());
    imageViewer->setWindowTitle(tr("Pinout"));
    imageViewer->resize(800,600);

    // Start the board interface
    jsonht = new BoardJson(&trkset);
    bno055 = new BoardBNO055(&trkset);

    // Add it to the list of available boards
    boards.append(jsonht);
    boards.append(bno055);

    // Once correct board is discovered this will be set to one of the above boards
    currentboard = nullptr;
    ui->tabBLE->setCurrentIndex(0);

    setWindowTitle(windowTitle() + " " + version);

    // Serial Connection
    serialcon = new QSerialPort;

    // Diagnostic Display + Serial Debug
    diagnostic = new DiagnosticDisplay(&trkset,this);
    diagnostic->setWindowFlags(Qt::Window);
    serialDebug = new QTextEdit(this);
    serialDebug->setWindowFlags(Qt::Window);
    serialDebug->setWindowTitle(tr("Serial Information"));
    serialDebug->resize(600,300);
    channelviewer = new ChannelViewer(&trkset, this);
    channelviewer->setWindowFlags(Qt::Window);

#ifdef DEBUG_HT
    serialDebug->show();
    diagnostic->show();
#endif

    // Hide these buttons until connected
    ui->cmdStartGraph->setVisible(false);
    ui->cmdStopGraph->setVisible(false);
    ui->chkRawData->setVisible(false);

    // Firmware loader loader dialog
    firmwareWizard = nullptr;

    // Get list of available ports
    findSerialPorts();

    // Use system default fixed witdh font
    serialDebug->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // Update default settings to UI
    updateToUI();

    // Board Connections, connects all boards signals to same end points
    foreach(BoardType *brd, boards) {
        connect(brd, &BoardType::paramSendStart, this,  &MainWindow::paramSendStart);
        connect(brd, &BoardType::paramSendComplete, this,  &MainWindow::paramSendComplete);
        connect(brd, &BoardType::paramSendFailure, this,  &MainWindow::paramSendFailure);
        connect(brd, &BoardType::paramReceiveStart, this,  &MainWindow::paramReceiveStart);
        connect(brd, &BoardType::paramReceiveComplete, this,  &MainWindow::paramReceiveComplete);
        connect(brd, &BoardType::paramReceiveFailure, this,  &MainWindow::paramReceiveFailure);
        connect(brd, &BoardType::calibrationSuccess, this,  &MainWindow::calibrationSuccess);
        connect(brd, &BoardType::calibrationFailure, this,  &MainWindow::calibrationFailure);
        connect(brd, &BoardType::serialTxReady, this,  &MainWindow::serialTxReady);
        connect(brd, &BoardType::addToLog,this, &MainWindow::addToLog);
        connect(brd, &BoardType::needsCalibration,this, &MainWindow::needsCalibration);
        connect(brd, &BoardType::boardDiscovered,this, &MainWindow::boardDiscovered);
        connect(brd, &BoardType::statusMessage,this, &MainWindow::statusMessage);
    }

    // Serial data ready
    connect(serialcon, &QSerialPort::readyRead,this, &MainWindow::serialReadReady);
    connect(serialcon, &QSerialPort::errorOccurred,this, &MainWindow::serialError);

    // Buttons
    connect(ui->cmdConnect,&QPushButton::clicked,this,&MainWindow::connectDisconnectClicked);
    connect(ui->cmdSend, &QPushButton::clicked,this, &MainWindow::manualSend);
    //connect(ui->cmdStartGraph, &QPushButton::clicked,this, &MainWindow::startGraph);
    //connect(ui->cmdStopGraph, &QPushButton::clicked,this, &MainWindow::stopGraph);
    connect(ui->cmdResetCenter, &QPushButton::clicked,this,  &MainWindow::resetCenter);
    connect(ui->cmdCalibrate, &QPushButton::clicked,this,  &MainWindow::startCalibration);
    connect(ui->cmdSaveNVM, &QPushButton::clicked,this, &MainWindow::storeToNVM);
    connect(ui->cmdReboot, &QPushButton::clicked,this, &MainWindow::reboot);
    connect(ui->cmdChannelViewer, &QPushButton::clicked, ui->actionChannel_Viewer, &QAction::trigger);
    connect(ui->cmdRefresh,&QPushButton::clicked,this,&MainWindow::findSerialPorts);

    // Check Boxes
    connect(ui->chkpanrev, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkrllrev, &QPushButton::clicked, this,&MainWindow::updateFromUI);
    connect(ui->chktltrev, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkInvertedPPM, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkInvertedPPMIn, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkResetCenterWave, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkSbusInInv, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkSbusOutInv, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkLngBttnPress, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkRstOnTlt, &QPushButton::clicked, this, &MainWindow::updateFromUI);
    connect(ui->chkCh5Arm, &QPushButton::clicked, this ,&MainWindow::updateFromUI);
    connect(ui->chkCRSFInv, &QPushButton::clicked, this, &MainWindow::updateFromUI);

    //connect(ui->chkRawData,&QPushButton::clicked,this, &MainWindow::setDataMode(bool)));

    // Spin Boxes
    connect(ui->spnPPMSync, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnPPMFrameLen, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA0Gain, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA0Off, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA1Gain, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA1Off, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA2Gain, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA2Off, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA3Gain, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnA3Off, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnRotX, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnRotY, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnRotZ, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnSBUSRate, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnCRSFRate, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);

    // Gain Sliders
    connect(ui->til_gain, &GainSlider::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->rll_gain, &GainSlider::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->pan_gain, &GainSlider::valueChanged, this, &MainWindow::updateFromUI);
    ui->til_gain->setMaximum(TrackerSettings::MAX_GAIN*10);
    ui->rll_gain->setMaximum(TrackerSettings::MAX_GAIN*10);
    ui->pan_gain->setMaximum(TrackerSettings::MAX_GAIN*10);

    // Servo Scaling Widgets
    connect(ui->servoPan, &ServoMinMax::minimumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoPan, &ServoMinMax::maximumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoPan, &ServoMinMax::centerChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoTilt, &ServoMinMax::minimumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoTilt, &ServoMinMax::maximumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoTilt, &ServoMinMax::centerChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoRoll, &ServoMinMax::minimumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoRoll, &ServoMinMax::maximumChanged, this, &MainWindow::updateFromUI);
    connect(ui->servoRoll, &ServoMinMax::centerChanged, this, &MainWindow::updateFromUI);

    // On Live Data Change
    connect(&trkset,&TrackerSettings::rawOrientChanged,this,&MainWindow::offOrientChanged);
    connect(&trkset,&TrackerSettings::offOrientChanged,this,&MainWindow::offOrientChanged);
    connect(&trkset,&TrackerSettings::ppmOutChanged,this,&MainWindow::ppmOutChanged);
    connect(&trkset,&TrackerSettings::liveDataChanged,this,&MainWindow::liveDataChanged);
    connect(&trkset,&TrackerSettings::bleAddressDiscovered,this,&MainWindow::bleAddressDiscovered);

    // Combo Boxes
        // Add Remap Choices + The corresponding values
    ui->cmbRemap->addItem("X,Y,Z",AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z));
    ui->cmbRemap->addItem("X,Z,Y",AXES_MAP(AXIS_X,AXIS_Z,AXIS_Y));
    ui->cmbRemap->addItem("Y,X,Z",AXES_MAP(AXIS_Y,AXIS_X,AXIS_Z));
    ui->cmbRemap->addItem("Y,Z,X",AXES_MAP(AXIS_Y,AXIS_Z,AXIS_X));
    ui->cmbRemap->addItem("Z,X,Y",AXES_MAP(AXIS_Z,AXIS_X,AXIS_Y));
    ui->cmbRemap->addItem("Z,Y,Z",AXES_MAP(AXIS_Z,AXIS_Y,AXIS_X));
    ui->cmbRemap->setCurrentIndex(0);

    connect(ui->cmbpanchn,  &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbtiltchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbrllchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbalertchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbRemap, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbSigns, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbButtonPin, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPpmInPin, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPpmOutPin, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbBtMode, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    //connect(ui->cmbResetOnPPM, &QComboBox::currentIndexChanged,this,&MainWindow::updateFromUI));
    connect(ui->cmbPPMChCount, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbA0Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbA1Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbA2Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbA3Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn0, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn1, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn2, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn0Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn1Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbAuxFn2Ch, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPWM0, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPWM1, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPWM2, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbPWM3, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbBTRmtMode, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbUartMode, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);

    // Menu Actions
    connect(ui->action_Save_to_File, &QAction::triggered,this, &MainWindow::saveSettings);
    connect(ui->action_Load, &QAction::triggered,this, &MainWindow::loadSettings);
    connect(ui->actionE_xit, &QAction::triggered,this, &MainWindow::close);
    connect(ui->actionFirmware_Wizard, &QAction::triggered,this, &MainWindow::uploadFirmwareWizard);
    connect(ui->actionShow_Data, &QAction::triggered,this, &MainWindow::showDiagsClicked);
    connect(ui->actionShow_Serial_Transmissions, &QAction::triggered,this, &MainWindow::showSerialDiagClicked);
    connect(ui->actionChannel_Viewer, &QAction::triggered,this, &MainWindow::showChannelViewerClicked);
    connect(ui->actionPinout,  &QAction::triggered,this,  &MainWindow::showPinView);
    connect(ui->actionEraseFlash,  &QAction::triggered,this,  &MainWindow::eraseFlash);
    connect(ui->actionOnline_Help,  &QAction::triggered,this,  &MainWindow::openHelp);
    connect(ui->actionDonate,  &QAction::triggered,this,  &MainWindow::openDonate);
    connect(ui->action_GitHub,  &QAction::triggered,this,  &MainWindow::openGitHub);
    connect(ui->action_Discord_Chat,  &QAction::triggered,this,  &MainWindow::openDiscord);

    // Tab Widget
    connect(ui->tabBLE,&QTabWidget::currentChanged,this,&MainWindow::BLE33tabChanged);

    // Timers
    connect(&rxledtimer, &QTimer::timeout,this, &MainWindow::rxledtimeout);
    rxledtimer.setInterval(100);
    connect(&txledtimer, &QTimer::timeout,this, &MainWindow::txledtimeout);
    txledtimer.setInterval(100);
    connect(&connectTimer, &QTimer::timeout,this, &MainWindow::connectTimeout);
    connectTimer.setSingleShot(true);
    connect(&requestTimer, &QTimer::timeout,this, &MainWindow::requestTimeout);
    requestTimer.setSingleShot(true);
    connect(&saveToRAMTimer, &QTimer::timeout,this, &MainWindow::saveToRAMTimeout);
    saveToRAMTimer.setSingleShot(true);
    connect(&requestParamsTimer, &QTimer::timeout,this, &MainWindow::requestParamsTimeout);
    requestParamsTimer.setSingleShot(true);

    // Set GIT SHA in bottom right of StatusBar
    ui->statusbar->addPermanentWidget(new QLabel(GIT_CURRENT_SHA_STRING));

    // Called to initalize GUI state to disconnected
    serialDisconnect();
}

MainWindow::~MainWindow()
{
    delete serialcon;
    delete jsonht;
    delete bno055;
    if(firmwareWizard != nullptr)
        delete firmwareWizard;
    delete serialDebug;
    delete ui;
}


void MainWindow::connectDisconnectClicked()
{
  if(serialcon->isOpen())
    serialDisconnect();
  else
    serialConnect();
}

// Connects to the serial port

void MainWindow::serialConnect()
{    
    QString port = ui->cmbPort->currentData().toString();
    if(port.isEmpty())
        return;

    // If open close it first
    if(serialcon->isOpen())
        serialDisconnect();

    ui->cmdConnect->setText(tr(" Disconnect"));
    ui->cmdConnect->setIcon(QIcon(":/Icons/images/disconnect.svg"));

    // Setup serial port 8N1, 57600 Baud
    serialcon->setPortName(port);
    serialcon->setParity(QSerialPort::NoParity);
    serialcon->setDataBits(QSerialPort::Data8);
    serialcon->setStopBits(QSerialPort::OneStop);
    serialcon->setBaudRate(QSerialPort::Baud115200); // CDC doesn't actuall make a dif.. cool
    serialcon->setFlowControl(QSerialPort::NoFlowControl);

    if(!serialcon->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this,tr("Error"),tr("Could not open Com ") + serialcon->portName());
        addToLog("Could not open " + serialcon->portName(),2);
        serialDisconnect();
        return;
    }

    logd.clear();
    serialDebug->clear();
    trkset.clear();

    ui->cmdChannelViewer->setEnabled(false);
    ui->actionEraseFlash->setEnabled(true);
    addToLog(tr("Connected to ") + serialcon->portName());
    statusMessage(tr("Connected to ") + serialcon->portName());

    ui->stackedWidget->setCurrentIndex(1);

    serialcon->setDataTerminalReady(true);

    requestTimer.stop();
    requestTimer.start(WAIT_FOR_BOARD_TO_BOOT);
}

// Disconnect from the serial port
// Reset all gui settings to disable mode

void MainWindow::serialDisconnect()
{
    if(serialcon->isOpen()) {
        // Check if user wants to save first
        if(!checkSaved())
            return;

        addToLog(tr("Disconnecting from ") + serialcon->portName());

        serialcon->flush();
        serialcon->close();
    }

    foreach(BoardType *brd, boards) {
        brd->_disconnected();
        brd->allowAccess(false);
    }

    statusMessage(tr("Disconnected"));
    ui->cmdConnect->setText(tr(" Connect"));
    ui->cmdConnect->setIcon(QIcon(":/Icons/images/connect.svg"));
    ui->cmdChannelViewer->setEnabled(false);
    ui->cmdStopGraph->setEnabled(false);
    ui->cmdStartGraph->setEnabled(false);
    ui->cmdSaveNVM->setEnabled(false);
    ui->cmdReboot->setEnabled(false);
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
    ui->cmdSend->setEnabled(false);
    ui->cmdCalibrate->setEnabled(false);
    ui->cmdResetCenter->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
    ui->servoPan->setShowActualPosition(false);
    ui->servoTilt->setShowActualPosition(false);
    ui->servoRoll->setShowActualPosition(false);
    ui->actionEraseFlash->setEnabled(false);

    sending = false;
    currentboard = nullptr;
    boardRequestIndex=0;
    connectTimer.stop();
    saveToRAMTimer.stop();
    requestTimer.stop();
    requestParamsTimer.stop();
    waitingOnParameters = false;
    serialData.clear();
    channelviewer->setBoard(nullptr);
    if(channelviewer->isVisible()) {
        channelViewerOpen = true;
        channelviewer->hide();
    } else {
        channelViewerOpen = false;
    }
    trkset.clearDataItems();
    bleaddrs.clear();
    ui->cmbBTRmtMode->clear();
    ui->cmbBTRmtMode->addItem(tr("First Available Device"));

    // Notify all boards we have disconnected
}

void MainWindow::serialError(QSerialPort::SerialPortError err)
{
    switch(err) {
    // Issue with connection - device unplugged
    case QSerialPort::ResourceError: {
        addToLog(tr("Connection lost"),2);
        serialcon->close();
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
        QMessageBox::information(this,tr("Error"), tr("No valid board detected\nPlease check COM port or flash proper code"));
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

    bool done = true;
    while(done) {
        int nlindex = data.indexOf("\r\n");
        if(nlindex < 0)
           break;  // No New line found

        // Strip data up the the CR LF \r\n
        QByteArray sdata = data.left(nlindex) + "\r\n";

        serialDataOut.enqueue(sdata);
        slowSerialSend();

        // Skip nuisance I'm here message
        if(QString(sdata).contains("{\"Cmd\":\"IH\"}"))
            return;

        addToLog("GUI: " + sdata + "\n");
        data = data.right(data.length()-nlindex-2);
    }

    ui->txled->setState(true);
    txledtimer.start();
}

void MainWindow::slowSerialSend()
{
    if(sending)
        return;

    sending = true;

    while(serialDataOut.length()) {
        QByteArray sdata = serialDataOut.dequeue();
        // Delay sends no more tha 64 bytes at a time
        int pos=0;
        while(pos<sdata.length()) {
            qDebug() << tr("Serial Out:") << sdata.mid(pos,64);
            serialcon->write(sdata.mid(pos,64));
            pos +=64;
            QTime dieTime= QTime::currentTime().addMSecs(5);
            while (QTime::currentTime() < dieTime)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }

    sending = false;
}

/* addToLog()
 *      Add other information to the LOG
 */

void MainWindow::addToLog(QString log, int ll)
{
    QString color = "black";
    if(ll==2) // Debug
        color = "red";
    else if(log.contains("HT:")) // TODO change this to detect logger messages
        color = "green";
    else if(log.contains("GUI:"))
        color = "blue";
    else if(log.contains("\"Cmd\":\"Data\"")) // Don't show return measurment data
        return;

    logd += "<font color=\"" + color + "\">" + log + "</font><br>";

    // Limit Max Log Length
    int loglen = logd.length();
    if(loglen > MAX_LOG_LENGTH)
        logd = logd.mid(logd.length() - MAX_LOG_LENGTH);

    // Set Gui text from local string
    serialDebug->setHtml(logd);

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
    int devfound=-1;
    ui->cmbPort->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    int i=0;
    foreach(QSerialPortInfo port,ports) {
        QString additional;
        if(port.vendorIdentifier() == 0x2341 &&
           port.productIdentifier() == 0x805A) { // NANO 33 BLE
            devfound = i;
            additional = " NanoBLE";
        }
        ui->cmbPort->addItem(port.portName() + additional, port.portName());
        i++;
    }
    if(devfound >= 0) {
        ui->cmbPort->setCurrentIndex(devfound);
    }
}

// Update the UI Settings from the settings class
void MainWindow::updateToUI()
{
    // Don't update GUI if haven't got data from the device yet
    if(waitingOnParameters)
        return;

    // Disable signals on all Comboboxes, SpinBoxes, Sliders
    // Prevents signals from these items causing another update
    QList<QComboBox *> comboWidgets = ui->centralwidget->findChildren<QComboBox*>();
    foreach(QComboBox *wid, comboWidgets)
        wid->blockSignals(true);
    QList<QSpinBox *> spinWidgets = ui->centralwidget->findChildren<QSpinBox*>();
    foreach(QSpinBox *wid, spinWidgets)
        wid->blockSignals(true);
    QList<QDoubleSpinBox *> spindWidgets = ui->centralwidget->findChildren<QDoubleSpinBox*>();
    foreach(QDoubleSpinBox *wid, spindWidgets)
        wid->blockSignals(true);
    QList<GainSlider *> sliderWidgets = ui->centralwidget->findChildren<GainSlider*>();
    foreach(GainSlider *wid, sliderWidgets)
        wid->blockSignals(true);

    ui->servoTilt->setCenter(trkset.getTlt_Cnt());
    ui->servoTilt->setMaximum(trkset.getTlt_Max());
    ui->servoTilt->setMinimum(trkset.getTlt_Min());

    ui->servoPan->setCenter(trkset.getPan_Cnt());
    ui->servoPan->setMaximum(trkset.getPan_Max());
    ui->servoPan->setMinimum(trkset.getPan_Min());

    ui->servoRoll->setCenter(trkset.getRll_Cnt());
    ui->servoRoll->setMaximum(trkset.getRll_Max());
    ui->servoRoll->setMinimum(trkset.getRll_Min());

    ui->chkpanrev->setChecked(trkset.isPanReversed() );
    ui->chkrllrev->setChecked(trkset.isRollReversed());
    ui->chktltrev->setChecked(trkset.isTiltReversed());
    ui->chkInvertedPPM->setChecked(trkset.getPpmOutInvert());
    ui->chkInvertedPPMIn->setChecked(trkset.getPpmInInvert());
    ui->chkResetCenterWave->setChecked(trkset.getRstOnWave());
    ui->chkSbusInInv->setChecked(trkset.getSbInInv());
    ui->chkSbusOutInv->setChecked(trkset.getSbOutInv());
    ui->chkLngBttnPress->setChecked(trkset.getButLngPs());
    ui->chkRstOnTlt->setChecked(trkset.getRstOnTlt());
    ui->chkCRSFInv->setChecked(trkset.getCrsfTxInv());

    // Button Press Mode - Enable/Disable on long press (Disable if no button pin selected)
    if(trkset.getButtonPin() > 0)
        ui->chkLngBttnPress->setEnabled(true);
    else
        ui->chkLngBttnPress->setEnabled(false);

    ui->spnPPMFrameLen->setMinimum((double)TrackerSettings::PPM_MIN_FRAME / 1000.0);
    ui->spnPPMFrameLen->setMaximum((double)TrackerSettings::PPM_MAX_FRAME / 1000.0);

    ui->spnA0Gain->setValue(trkset.getAn0Gain());
    ui->spnA0Off->setValue(trkset.getAn0Off());
    ui->spnA1Gain->setValue(trkset.getAn1Gain());
    ui->spnA1Off->setValue(trkset.getAn1Off());
    ui->spnA2Gain->setValue(trkset.getAn2Gain());
    ui->spnA2Off->setValue(trkset.getAn2Off());
    ui->spnA3Gain->setValue(trkset.getAn3Gain());
    ui->spnA3Off->setValue(trkset.getAn3Off());

    ui->spnSBUSRate->setValue(trkset.getSbusTxRate());
    ui->spnCRSFRate->setValue(trkset.getCrsfTxRate());

    int panCh = trkset.getPanCh();
    int rllCh = trkset.getRllCh();
    int tltCh = trkset.getTltCh();
    int alertCh = trkset.getAlertCh();
    int a0Ch = trkset.getAn0Ch();
    int a1Ch = trkset.getAn1Ch();
    int a2Ch = trkset.getAn2Ch();
    int a3Ch = trkset.getAn3Ch();
    int auxF0Ch = trkset.getAux0Ch();
    int auxF1Ch = trkset.getAux1Ch();
    int auxF2Ch = trkset.getAux2Ch();
    int pwm0Ch = trkset.getPwm0();
    int pwm1Ch = trkset.getPwm1();
    int pwm2Ch = trkset.getPwm2();
    int pwm3Ch = trkset.getPwm3();

    // Tilt/Rll/Pan Ch
    ui->cmbpanchn->setCurrentIndex(panCh==-1?0:panCh);
    ui->cmbrllchn->setCurrentIndex(rllCh==-1?0:rllCh);
    ui->cmbtiltchn->setCurrentIndex(tltCh==-1?0:tltCh);
    ui->cmbalertchn->setCurrentIndex(alertCh==-1?0:alertCh);
    // Uart Mode
    ui->cmbUartMode->setCurrentIndex(trkset.getUartMode());
    ui->stkUart->setCurrentIndex(trkset.getUartMode());
    ui->chkCh5Arm->setChecked(trkset.getCh5Arm());
    // Analog CH
    ui->cmbA0Ch->setCurrentIndex(a0Ch==-1?0:a0Ch);
    ui->cmbA1Ch->setCurrentIndex(a1Ch==-1?0:a1Ch);
    ui->cmbA2Ch->setCurrentIndex(a2Ch==-1?0:a2Ch);
    ui->cmbA3Ch->setCurrentIndex(a3Ch==-1?0:a3Ch);
    // Aux Funcs
    ui->cmbAuxFn0Ch->setCurrentIndex(auxF0Ch==-1?0:auxF0Ch);
    ui->cmbAuxFn1Ch->setCurrentIndex(auxF1Ch==-1?0:auxF1Ch);
    ui->cmbAuxFn2Ch->setCurrentIndex(auxF2Ch==-1?0:auxF2Ch);
    ui->cmbAuxFn0->setCurrentIndex(trkset.getAux0Func());
    ui->cmbAuxFn1->setCurrentIndex(trkset.getAux1Func());
    ui->cmbAuxFn2->setCurrentIndex(trkset.getAux2Func());

    // PWM Chs
    ui->cmbPWM0->setCurrentIndex(pwm0Ch==-1?0:pwm0Ch);
    ui->cmbPWM1->setCurrentIndex(pwm1Ch==-1?0:pwm1Ch);
    ui->cmbPWM2->setCurrentIndex(pwm2Ch==-1?0:pwm2Ch);
    ui->cmbPWM3->setCurrentIndex(pwm3Ch==-1?0:pwm3Ch);

    ui->cmbRemap->setCurrentIndex(ui->cmbRemap->findData(trkset.axisRemap()));
    ui->cmbSigns->setCurrentIndex(trkset.axisSign());
    ui->cmbBtMode->setCurrentIndex(trkset.getBtMode());
    int rot[3];
    trkset.orientation(rot[0],rot[1],rot[2]);
    ui->spnRotX->setValue(rot[0]);
    ui->spnRotY->setValue(rot[1]);
    ui->spnRotZ->setValue(rot[2]);

    ui->til_gain->setValue(trkset.getTlt_Gain()*10);
    ui->pan_gain->setValue(trkset.getPan_Gain()*10);
    ui->rll_gain->setValue(trkset.getRll_Gain()*10);

    int ppout_index = trkset.getPpmOutPin()-1;
    int ppin_index = trkset.getPpmInPin()-1;
    int but_index = trkset.getButtonPin()-1;
    //int resppm_index = trkset.resetCntPPM();
    ui->cmbPpmOutPin->setCurrentIndex(ppout_index < 1 ? 0 : ppout_index);
    ui->cmbPpmInPin->setCurrentIndex(ppin_index < 1 ? 0 : ppin_index);
    ui->cmbButtonPin->setCurrentIndex(but_index < 1 ? 0 : but_index);
    //ui->cmbResetOnPPM->setCurrentIndex(resppm_index < 0 ? 0: resppm_index);

    // PPM Output Settings
    int channels = trkset.getPpmChCnt();
    ui->cmbPPMChCount->setCurrentIndex(channels-1);
    uint16_t setframelen = trkset.getPpmFrame();
    ui->spnPPMFrameLen->setValue(static_cast<double>(setframelen)/1000.0f);
    ui->spnPPMSync->setValue(trkset.getPpmSync());
    uint32_t maxframelen = TrackerSettings::PPM_MIN_FRAMESYNC + (channels * TrackerSettings::MAX_PWM);
    if(maxframelen > setframelen) {
        ui->lblPPMOut->setText(tr("<b>Warning!</b> PPM Frame length possibly too short to support channel data"));
    } else {
        ui->lblPPMOut->setText(tr("PPM data will fit in frame. Refresh rate: ") + QString::number(1/(static_cast<float>(setframelen)/1000000.0),'f',2) + " Hz");
    }

    // BT Pair Address
    if(trkset.getBtPairedAddress().isEmpty()) {
        ui->cmbBTRmtMode->setCurrentIndex(0);
    } else {
        bleAddressDiscovered(trkset.getBtPairedAddress());
        ui->cmbBTRmtMode->setCurrentText(trkset.getBtPairedAddress());
    }

    if(ui->cmbBtMode->currentIndex() > 1) // Remote or Scanner Mode
    {
        ui->lblPairWith->setVisible(true);
        ui->cmbBTRmtMode->setVisible(true);
    } else {
        ui->lblPairWith->setVisible(false);
        ui->cmbBTRmtMode->setVisible(false);
    }

    // Allow signals again
    comboWidgets = ui->centralwidget->findChildren<QComboBox*>();
    foreach(QComboBox *wid, comboWidgets)
        wid->blockSignals(false);
    spinWidgets = ui->centralwidget->findChildren<QSpinBox*>();
    foreach(QSpinBox *wid, spinWidgets)
        wid->blockSignals(false);
    spindWidgets = ui->centralwidget->findChildren<QDoubleSpinBox*>();
    foreach(QDoubleSpinBox *wid, spindWidgets)
        wid->blockSignals(false);
    sliderWidgets = ui->centralwidget->findChildren<GainSlider*>();
    foreach(GainSlider *wid, sliderWidgets)
        wid->blockSignals(false);
}

// Update the Settings Class from the UI Data
void MainWindow::updateFromUI()
{
    if(!serialcon->isOpen()) // Don't update anything if not connected
        return;

    // Pan Tilt Roll
    int panCh = ui->cmbpanchn->currentIndex();
    int rllCh = ui->cmbrllchn->currentIndex();
    int tltCh = ui->cmbtiltchn->currentIndex();
    int alertCh = ui->cmbalertchn->currentIndex();
    trkset.setPanCh(panCh==0?-1:panCh);
    trkset.setRllCh(rllCh==0?-1:rllCh);
    trkset.setTltCh(tltCh==0?-1:tltCh);
    trkset.setAlertCh(alertCh==0?-1:alertCh);
    trkset.setRollReversed(ui->chkrllrev->isChecked());
    trkset.setPanReversed(ui->chkpanrev->isChecked());
    trkset.setTiltReversed(ui->chktltrev->isChecked());
    trkset.setPan_Cnt(ui->servoPan->centerValue());
    trkset.setPan_Min(ui->servoPan->minimumValue());
    trkset.setPan_Max(ui->servoPan->maximumValue());
    trkset.setPan_Gain(static_cast<float>(ui->pan_gain->value())/10.0f);
    trkset.setTlt_Cnt(ui->servoTilt->centerValue());
    trkset.setTlt_Min(ui->servoTilt->minimumValue());
    trkset.setTlt_Max(ui->servoTilt->maximumValue());
    trkset.setTlt_Gain(static_cast<float>(ui->til_gain->value())/10.0f);
    trkset.setRll_Cnt(ui->servoRoll->centerValue());
    trkset.setRll_Min(ui->servoRoll->minimumValue());
    trkset.setRll_Max(ui->servoRoll->maximumValue());
    trkset.setRll_Gain(static_cast<float>(ui->rll_gain->value())/10.0f);

    // Uart Mode
    trkset.setUartMode(ui->cmbUartMode->currentIndex());
    trkset.setCh5Arm(ui->chkCh5Arm->isChecked());

    // Analog
    trkset.setAn0Gain(ui->spnA0Gain->value());
    trkset.setAn0Off(ui->spnA0Off->value());
    trkset.setAn1Gain(ui->spnA1Gain->value());
    trkset.setAn1Off(ui->spnA1Off->value());
    trkset.setAn2Gain(ui->spnA2Gain->value());
    trkset.setAn2Off(ui->spnA2Off->value());
    trkset.setAn3Gain(ui->spnA3Gain->value());
    trkset.setAn3Off(ui->spnA3Off->value());
    int an0Ch = ui->cmbA0Ch->currentIndex();
    int an1Ch = ui->cmbA1Ch->currentIndex();
    int an2Ch = ui->cmbA2Ch->currentIndex();
    int an3Ch = ui->cmbA3Ch->currentIndex();
    trkset.setAn0Ch(an0Ch==0?-1:an0Ch);
    trkset.setAn1Ch(an1Ch==0?-1:an1Ch);
    trkset.setAn2Ch(an2Ch==0?-1:an2Ch);
    trkset.setAn3Ch(an3Ch==0?-1:an3Ch);

    // Aux
    int auxF0Ch = ui->cmbAuxFn0Ch->currentIndex();
    int auxF1Ch = ui->cmbAuxFn1Ch->currentIndex();
    int auxF2Ch = ui->cmbAuxFn2Ch->currentIndex();
    trkset.setAux0Ch(auxF0Ch==0?-1:auxF0Ch);
    trkset.setAux1Ch(auxF1Ch==0?-1:auxF1Ch);
    trkset.setAux2Ch(auxF2Ch==0?-1:auxF2Ch);
    trkset.setAux0Func(ui->cmbAuxFn0->currentIndex());
    trkset.setAux1Func(ui->cmbAuxFn1->currentIndex());
    trkset.setAux2Func(ui->cmbAuxFn2->currentIndex());

    // PWM
    int pwmCh0 = ui->cmbPWM0->currentIndex();
    int pwmCh1 = ui->cmbPWM1->currentIndex();
    int pwmCh2 = ui->cmbPWM2->currentIndex();
    int pwmCh3 = ui->cmbPWM3->currentIndex();
    trkset.setPwm0(pwmCh0==0?-1:pwmCh0);
    trkset.setPwm1(pwmCh1==0?-1:pwmCh1);
    trkset.setPwm2(pwmCh2==0?-1:pwmCh2);
    trkset.setPwm3(pwmCh3==0?-1:pwmCh3);

    // BNO Axis Remapping
    trkset.setAxisRemap(ui->cmbRemap->currentData().toUInt());
    trkset.setAxisSign(ui->cmbSigns->currentIndex());


    // Check all pins for duplicates

    // Shift the index of the disabled choice to -1 in settings
    enum {PIN_PPMIN,PIN_PPMOUT,PIN_BUTRESET,PIN_SBUSIN1,PIN_SBUSIN2};
    int pins[5] {-1,-1,-1,-1,-1};

    int ppout_index = ui->cmbPpmOutPin->currentIndex()+1;
    pins[PIN_PPMOUT] = ppout_index==1?-1:ppout_index;
    int ppin_index = ui->cmbPpmInPin->currentIndex()+1;
    pins[PIN_PPMIN] = ppin_index==1?-1:ppin_index;
    int but_index = ui->cmbButtonPin->currentIndex()+1;
    pins[PIN_BUTRESET] = but_index==1?-1:but_index;

    bool sbusinchecked = ui->chkSbusInInv->isChecked();
    if(!sbusinchecked) {
        pins[PIN_SBUSIN1] = 5;
        pins[PIN_SBUSIN2] = 6;
    }

    // Loop through all possibilites checking for duplicates
    bool duplicates=false;
    for(int i=0; i < 4; i++) {
        for(int y=i+1; y < 5; y++) {
            if(pins[i] > 0 && pins[y] > 0 && pins[i] == pins[y]) {
                duplicates = true;
                break;
            }
        }
    }

    // Check for pin duplicates
    if(duplicates) {
        QString message = tr("Cannot pick dulplicate pins");
        if(!sbusinchecked)
            message += tr("\n  * non-inverted SBUS receive requires  D5 and D6 connected together");
        QMessageBox::information(this,tr("Error"), message);

        // Reset gui to old values
        ppout_index = trkset.getPpmOutPin()-1;
        ppin_index = trkset.getPpmInPin()-1;
        but_index = trkset.getButtonPin()-1;
        sbusinchecked = trkset.getSbInInv();
        ui->cmbPpmOutPin->setCurrentIndex(ppout_index < 1 ? 0 : ppout_index);
        ui->cmbPpmInPin->setCurrentIndex(ppin_index < 1 ? 0 : ppin_index);
        ui->cmbButtonPin->setCurrentIndex(but_index < 1 ? 0 : but_index);
        ui->chkSbusInInv->setChecked(sbusinchecked);

    } else {
        trkset.setPpmOutPin(pins[PIN_PPMOUT]);
        trkset.setPpmInPin(pins[PIN_PPMIN]);
        trkset.setButtonPin(pins[PIN_BUTRESET]);
        trkset.setSbInInv(sbusinchecked);
    }

    // Button Press Mode - Enable/Disable on long press (Disable if no button pin selected)
    if(trkset.getButtonPin() > 0)
        ui->chkLngBttnPress->setEnabled(true);
    else
        ui->chkLngBttnPress->setEnabled(false);

    trkset.setButLngPs(ui->chkLngBttnPress->isChecked());
    trkset.setRstOnTlt(ui->chkRstOnTlt->isChecked());

    trkset.setSbOutInv(ui->chkSbusOutInv->isChecked());
    trkset.setSbusTxRate(ui->spnSBUSRate->value());
    trkset.setCrsfTxRate(ui->spnCRSFRate->value());
    trkset.setCrsfTxInv(ui->chkCRSFInv->isChecked());

    uint16_t setframelen = ui->spnPPMFrameLen->value() * 1000;
    trkset.setPpmFrame(setframelen);
    trkset.setPpmSync(ui->spnPPMSync->value());
    int channels = ui->cmbPPMChCount->currentIndex()+1;
    trkset.setPpmChCnt(channels);
    uint32_t maxframelen = TrackerSettings::PPM_MIN_FRAMESYNC + (channels * TrackerSettings::MAX_PWM);
    if(maxframelen > setframelen) {
        ui->lblPPMOut->setText(tr("<b>Warning!</b> PPM Frame length possibly too short to support channel data"));
    } else {
        ui->lblPPMOut->setText(tr("PPM data will fit in frame. Refresh rate: ") + QString::number(1/(static_cast<float>(setframelen)/1000000.0),'f',2) + " Hz");
    }

    //int rstppm_index = ui->cmbResetOnPPM->currentIndex();
  //  trkset.setResetCntPPM(rstppm_index==0?-1:rstppm_index);

    trkset.setBtMode(ui->cmbBtMode->currentIndex());
    if(ui->cmbBtMode->currentIndex() > 1) // Remote or Scanner Mode
    {
        ui->lblPairWith->setVisible(true);
        ui->cmbBTRmtMode->setVisible(true);
    } else {
        ui->lblPairWith->setVisible(false);
        ui->cmbBTRmtMode->setVisible(false);
    }

    if(ui->cmbBTRmtMode->currentIndex() == 0) {
        trkset.setBtPairedAddress(QString());
    } else {
        trkset.setBtPairedAddress(ui->cmbBTRmtMode->currentText().simplified());
    }

    trkset.setOrientation(ui->spnRotX->value(),
                          ui->spnRotY->value(),
                          ui->spnRotZ->value());

    trkset.setPpmOutInvert(ui->chkInvertedPPM->isChecked());
    trkset.setPpmInInvert(ui->chkInvertedPPMIn->isChecked());
    trkset.setRstOnWave(ui->chkResetCenterWave->isChecked());

    ui->cmdSaveNVM->setEnabled(true);

    // Use timer to prevent too many writes while drags, etc.. happen
    saveToRAMTimer.start(500);
}

// Data ready to be read from the serial port
void MainWindow::serialReadReady()
{
    // Receive LED
    ui->rxled->setState(true);
    rxledtimer.start();

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
        qDebug() << "Serial In:" << data;

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
    ui->lblTiltValue->setText(QString::number(t,'f',1) + "°");
    ui->lblRollValue->setText(QString::number(r,'f',1) + "°");
    ui->lblPanValue->setText(QString::number(p,'f',1) + "°");
}

void MainWindow::ppmOutChanged(int t,int r,int p)
{
    ui->servoTilt->setActualPosition(t);
    ui->servoRoll->setActualPosition(r);
    ui->servoPan->setActualPosition(p);

    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
}

void MainWindow::bleAddressDiscovered(QString str)
{
    if(bleaddrs.contains(str))
        return;
    bleaddrs.append(str);

    ui->cmbBTRmtMode->addItem(str);
}

void MainWindow::liveDataChanged()
{
    ui->lblBLEAddress->setText(trkset.getDataBtAddr());
    ui->btLed->setState(trkset.getDataBtCon());
    if(trkset.getDataBtCon())
        ui->lblBTConnected->setText(tr("Connected"));
    else
        ui->lblBTConnected->setText(tr("Not connected"));
    if(trkset.getBtMode() == TrackerSettings::BTDISABLE)
        ui->lblBTConnected->setText("Disabled");
    if(trkset.getDataTrpEnabled()) {
      ui->servoPan->setShowActualPosition(true);
      ui->servoTilt->setShowActualPosition(true);
      ui->servoRoll->setShowActualPosition(true);
      ui->lblRange->setText(tr("Range"));
      ui->lblRange->setToolTip("");
    } else {
      ui->servoPan->setShowActualPosition(false);
      ui->servoTilt->setShowActualPosition(false);
      ui->servoRoll->setShowActualPosition(false);
      ui->lblRange->setText(tr("<b>Range - Output Disabled</b>"));
    }
    if(trkset.getDataGyroCal())
      ui->gyroLed->setState(true);
    else
      ui->gyroLed->setState(false);
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

    // Re-enable data if window was open too long
    startData();
}

void MainWindow::loadSettings()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this, tr("Info"),tr("Please connect before restoring a saved file"));
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this,tr("Open File"),QString(),tr("Config Files (*.ini)"));

    // Re-enable data if window was open too long
    startData();

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
            QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not sent"),tr("Are you sure you want to disconnect?\n"\
                                  "Changes haven't been sent to the headtracker\nClick \"Send Changes\" first"),QMessageBox::Yes|QMessageBox::No);
            if(rval != QMessageBox::Yes)
                return false;

        } else if (!brd->_isBoardSavedToNVM()) {
            QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not saved on tracker"),tr("Are you sure you want to disconnect?\n"\
                                  "Changes haven't been permanently stored on headtracker\nClick \"Save settings\" first"),QMessageBox::Yes|QMessageBox::No);
            if(rval != QMessageBox::Yes)
                return false;
        }
    }
    return true;
}

void MainWindow::uploadFirmwareWizard()
{
    if(firmwareWizard == nullptr) {
        firmwareWizard = new FirmwareWizard(this);
        connect(firmwareWizard, &FirmwareWizard::programmingComplete, this, &MainWindow::findSerialPorts);
        firmwareWizard->setWindowFlags(Qt::Window);
    }
    firmwareWizard->show();
    firmwareWizard->activateWindow();
    firmwareWizard->raise();
}

/* requestTimeout()
 *      This timeout is called after waiting for the board to boot
 * it requests the hardware and version from the board, gives each board
 * 300ms to respond before trying the next one. Sets the allowAccess so
 * other boards won't respond on the serial line at the same time.
 */

void MainWindow::requestTimeout()
{
    // Was a board discovered, if so just quit
    if(currentboard != nullptr)
        return;

    // Otherwise increment to the next board and try again
    if(boardRequestIndex == boards.length()) {
        msgbox->setText(tr("Was unable to determine the board type\n\nHave you written the firmware to the board yet?"));
        msgbox->setWindowTitle("Error");
        msgbox->show();
        statusMessage(tr("Board discovery failed"));
        serialDisconnect();
        return;
    }

    // Prevent last board class from interfering
    if(boardRequestIndex > 0)
        boards[boardRequestIndex-1]->allowAccess(false);

    // Request hardware information from the new board
    addToLog(tr("Trying to connect to ") + boards[boardRequestIndex]->boardName() + "\n");
    boards[boardRequestIndex]->allowAccess(true);
    boards[boardRequestIndex]->requestHardware();
    requestTimer.start(WAIT_BETWEEN_BOARD_CONNECTIONS);

    // Move to next board
    boardRequestIndex++;
}

void MainWindow::saveToRAMTimeout()
{
    // Request hardware from all board types
    foreach(BoardType *brd, boards) {
        brd->_saveToRAM();
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

/**
 * @brief MainWindow::eraseFlash This function will erase all configuration and reset to default
 */
void MainWindow::eraseFlash()
{
  if(currentboard && (currentboard->boardName() == "NANO33BLE" ||
                      currentboard->boardName() == "DTQSYS")) {
    if(QMessageBox::question(this, tr("Set Defaults?"), tr("This will erase all settings to defaults\r\nAre you sure?")) == QMessageBox::Yes) {
      currentboard->_erase();
      currentboard->_reboot();
      QTime dieTime= QTime::currentTime().addMSecs(RECONNECT_AFT_REBT);
      while (QTime::currentTime() < dieTime)
          QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

      serialConnect();
    }
  }
}

// Start the various calibration dialogs
void MainWindow::startCalibration()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this,tr("Not Connected"), tr("Connect before trying to calibrate"));
        return;
    }

    foreach(BoardType *brd, boards) {
        brd->_startCalibration();
    }
}

// Tell the board to start sending data again
void MainWindow::startData()
{
    foreach(BoardType *brd, boards) {
        brd->_startData();
    }
}

void MainWindow::storeToNVM()
{
    foreach(BoardType *brd, boards) {
        brd->_saveToNVM();
    }
    statusMessage(tr("Storing Parameters to memory"));
    ui->cmdSaveNVM->setEnabled(false);
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

void MainWindow::showChannelViewerClicked()
{
    channelviewer->show();
    channelviewer->activateWindow();
    channelviewer->raise();
}

void MainWindow::BLE33tabChanged()
{
    if(currentboard == nullptr)
        return;

    QMap<QString,bool> dataitms;
    dataitms["tiltoff"] = true;
    dataitms["rolloff"] = true;
    dataitms["panoff"] = true;
    dataitms["tiltout"] = true;
    dataitms["rollout"] = true;
    dataitms["panout"] = true;
    dataitms["btcon"] = false;
    dataitms["gyrocal"] = false;
    dataitms["btaddr"] = false;
    dataitms["btrmt"] = false;

    switch(ui->tabBLE->currentIndex()) {
    case 0: { // General
        dataitms["gyrocal"] = true;
        break;
    }
    case 1: { // Output
        break;
    }
    case 2: { // PPM In
        break;
    }
    case 3: { // Bluetooth
        dataitms["btcon"] = true;
        dataitms["btrmt"] = true;
        dataitms["btaddr"] = true;
        break;
    }
    case 4: { // PWM
        break;
    }
    case 5: { // Extras
        break;
    }
    default:
        break;
    }

    trkset.setDataItemSend(dataitms);
}

/*void MainWindow::BTModeChanged()
{
    updateFromUI();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Reboot Required"),
                                  tr("Bluetooth mode change requires reboot.\n"
                                  "Save and reboot now?"),
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes) {
        storeToNVM();
        QTime dieTime= QTime::currentTime().addMSecs(800);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        reboot();
    }
}*/

void MainWindow::reboot()
{
    foreach(BoardType *brd, boards) {
        bool reboot=true;
        if(!brd->isAccessAllowed())
            continue;
        if(!brd->_isBoardSavedToNVM()) {
            QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not saved"),tr("Are you sure you want to reboot?\n"\
                                  "Changes haven't been saved\nClick \"Save Settings\" first"),QMessageBox::Yes|QMessageBox::No);
            if(rval != QMessageBox::Yes) {
                reboot = false;
            }
        }
        if(reboot) {
            brd->_reboot();
            QTime dieTime= QTime::currentTime().addMSecs(RECONNECT_AFT_REBT);
            while (QTime::currentTime() < dieTime)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

            serialConnect();
        }
    }
}

void MainWindow::openHelp()
{
  QDesktopServices::openUrl(helpurl);
}

void MainWindow::openDiscord()
{
  QDesktopServices::openUrl(discordurl);
}

void MainWindow::openDonate()
{
  QDesktopServices::openUrl(donateurl);
}

void MainWindow::openGitHub()
{
  QDesktopServices::openUrl(githuburl);
}

void MainWindow::showPinView()
{
    imageViewer->show();
}

void MainWindow::paramSendStart()
{
    statusMessage(tr("Starting parameter send"),3000);
}

void MainWindow::paramSendComplete()
{
    statusMessage(tr("Parameter(s) saved"), 5000);
}

void MainWindow::paramSendFailure(int)
{
    msgbox->setText(tr("Unable to upload the parameter(s))"));
    msgbox->setWindowTitle(tr("Error"));
    msgbox->show();
    statusMessage(tr("Parameters Send Failure"));
    serialDisconnect();
}

void MainWindow::paramReceiveStart()
{
    statusMessage(tr("Parameters Request Started"),5000);
}

void MainWindow::paramReceiveComplete()
{
    statusMessage(tr("Parameters Request Complete"),5000);
    waitingOnParameters = false;
    updateToUI();
    BLE33tabChanged(); // Request Data to be sent
    trkset.setDataItemSend("isCalibrated",true);
    trkset.setDataItemSend("trpenabled",true);
}

void MainWindow::paramReceiveFailure(int)
{
    msgbox->setText(tr("Unable to receive the parameters"));
    msgbox->setWindowTitle("Error");
    msgbox->show();
    statusMessage("Parameter Received Failure");
    serialDisconnect();
}

void MainWindow::calibrationSuccess()
{
    statusMessage(tr("Calibration Success"),5000);
}

void MainWindow::calibrationFailure()
{
    statusMessage(tr("Calibration Failure"),5000);
}

void MainWindow::serialTxReady()
{
    foreach(BoardType *brd, boards) {
        sendSerialData(brd->_dataout());
    }
}

void MainWindow::needsCalibration()
{
    msgbox->setText(tr("Calibration has not been performed.\nPlease calibrate or restore from a saved file"));
    msgbox->setWindowTitle(tr("Calibrate"));
    msgbox->show();
}

void MainWindow::boardDiscovered(BoardType *brd)
{
    // Board discovered, save it
    currentboard = brd;

    // GUI changes info depending on board type
    if(brd->boardName() == "NANO33BLE" ||
       brd->boardName() == "DTQSYS" ||
       brd->boardName() == "XIAOSENSE" ) {
        addToLog(tr("Connected to a ") + brd->boardName() + "\n");
        ui->cmdStartGraph->setVisible(false);
        ui->cmdStopGraph->setVisible(false);
        ui->chkRawData->setVisible(false);
        ui->cmbRemap->setVisible(false);
        ui->cmbSigns->setVisible(false);
        ui->cmdSaveNVM->setVisible(true);
        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        ui->cmdResetCenter->setEnabled(true);
        ui->cmdReboot->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(3);
        ui->cmdChannelViewer->setEnabled(true);

        if(brd->boardName() == "DTQSYS" ||
           brd->boardName() == "XIAOSENSE") { // Pins are all fixed
            ui->cmbPpmInPin->setVisible(false);
            ui->lblPPMInPin->setVisible(false);
            ui->cmbPpmOutPin->setVisible(false);
            ui->lblPPMOutPin->setVisible(false);
            ui->cmbButtonPin->setVisible(false);
            ui->lblButtonPin->setVisible(false);
            ui->tabBLE->setTabVisible(4,false);
            ui->lblAn4->setText(tr("Battery Voltage"));
            ui->lblAn5->setText(tr("Analog 1 (0.29)"));
            ui->lblAn6->setText(tr("Analog 2 (0.02)"));
            ui->lblAn7->setText(tr("Analog 3 (0.28)"));
        } else {
            ui->cmbPpmInPin->setVisible(true);
            ui->lblPPMInPin->setVisible(true);
            ui->cmbPpmOutPin->setVisible(true);
            ui->lblPPMOutPin->setVisible(true);
            ui->cmbButtonPin->setVisible(true);
            ui->lblButtonPin->setVisible(true);
            ui->tabBLE->setTabVisible(4,true);
            ui->lblAn4->setText(tr("Analog A4"));
            ui->lblAn5->setText(tr("Analog A5"));
            ui->lblAn6->setText(tr("Analog A6"));
            ui->lblAn7->setText(tr("Analog A7"));
        }

        // Check Firmware Version is Compatible

        // GUI Version
        QString lfver = version;
        lfver.remove(0,1);
        lfver.remove(1,1);
        int lmajver = lfver.left(2).toInt();   // Major Version 1.1x == 11

        // Remote Version
        int rmajver = trkset.fwVersion().remove(1,1).left(2).toInt();    // Major Version 1.1x == 11

        // Firmware is too old
        if(lmajver > rmajver) {
            msgbox->setText(tr("Firmware is outdated. Upload a firmware of ") + QString::number((float)lmajver/10,'f',1) + "x for this GUI");
            msgbox->setWindowTitle(tr("Firmware Version Mismatch"));
            msgbox->show();

        // Firmware is too new
        } else if (lmajver < rmajver) {
            msgbox->setText(tr("Firmware is newer than supported by this GUI. Download ") + QString::number((float)rmajver/10,'f',1) +"x \n\nfrom www.github.com/dlktdr/headtracker");
            msgbox->setWindowTitle(tr("Firmware Version Mismatch"));
            msgbox->show();
        }
        channelviewer->setBoard(currentboard);
        if(channelViewerOpen) {
            channelviewer->show();
        }
    } else if (brd->boardName() == "BNO055") {
        addToLog(tr("Connected to a ") + brd->boardName() + "\n");
        ui->cmdStartGraph->setVisible(true);
        ui->cmdStopGraph->setVisible(true);
        ui->cmbRemap->setVisible(true);
        ui->cmbSigns->setVisible(true);
        ui->chkRawData->setVisible(true);
        ui->cmdSaveNVM->setVisible(false);
        ui->cmdStopGraph->setEnabled(true);
        ui->cmdStartGraph->setEnabled(true);
        ui->cmdSend->setEnabled(true);
        ui->cmdSaveNVM->setEnabled(true);
        ui->cmdCalibrate->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(2);
    } else {
        msgbox->setText(tr("Unknown board type"));
        msgbox->setWindowTitle(tr("Error"));
        msgbox->show();
        statusMessage(tr("Unknown board type"));
        serialDisconnect();
    }

    requestParamsTimer.stop();
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

