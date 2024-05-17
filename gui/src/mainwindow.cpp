
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

    // Board interface
    boardDiscover = false;
    boardDiscoveryStarted = false;
    jsonht = new BoardJson(&trkset);

    // Once correct board is discovered this will be set to one of the above boards
    ui->tabBLE->setCurrentIndex(0);

    QString guiDisplayVersion = version;
    guiDisplayVersion.remove(4,1);
    setWindowTitle(windowTitle() + " " + guiDisplayVersion);

    // Serial Connection
    serialcon = new QSerialPort;

    // Diagnostic Display + Serial Debug
    diagnostic = new DiagnosticDisplay(&trkset, jsonht, this);
    diagnostic->setWindowFlags(Qt::Window);
    serialDebug = new QTextEdit(this);
    serialDebug->setWindowFlags(Qt::Window);
    serialDebug->setWindowTitle(tr("Serial Information"));
    serialDebug->setLineWrapMode(QTextEdit::NoWrap);
    serialDebug->resize(600,300);
    channelviewer = new ChannelViewer(&trkset, this);
    channelviewer->setWindowFlags(Qt::Window);

#ifdef DEBUG_HT
    serialDebug->show();
    diagnostic->show();
#endif

    // Firmware loader loader dialog
    firmwareWizard = nullptr;

    // Get list of available ports
    findSerialPorts();

    // Use system default fixed witdh font
    serialDebug->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // Update default settings to UI
    updateToUI();

    connect(jsonht, &BoardJson::paramSendStart, this,  &MainWindow::paramSendStart);
    connect(jsonht, &BoardJson::paramSendComplete, this,  &MainWindow::paramSendComplete);
    connect(jsonht, &BoardJson::paramSendFailure, this,  &MainWindow::paramSendFailure);
    connect(jsonht, &BoardJson::paramReceiveStart, this,  &MainWindow::paramReceiveStart);
    connect(jsonht, &BoardJson::paramReceiveComplete, this,  &MainWindow::paramReceiveComplete);
    connect(jsonht, &BoardJson::paramReceiveFailure, this,  &MainWindow::paramReceiveFailure);
    connect(jsonht, &BoardJson::featuresReceiveStart, this,  &MainWindow::featuresReceiveStart);
    connect(jsonht, &BoardJson::featuresReceiveComplete, this,  &MainWindow::featuresReceiveComplete);
    connect(jsonht, &BoardJson::featuresReceiveFailure, this,  &MainWindow::featuresReceiveFailure);
    connect(jsonht, &BoardJson::calibrationSuccess, this,  &MainWindow::calibrationSuccess);
    connect(jsonht, &BoardJson::calibrationFailure, this,  &MainWindow::calibrationFailure);
    connect(jsonht, &BoardJson::serialTxReady, this,  &MainWindow::serialTxReady);
    connect(jsonht, &BoardJson::addToLog,this, &MainWindow::addToLog);
    connect(jsonht, &BoardJson::needsCalibration,this, &MainWindow::needsCalibration);
    connect(jsonht, &BoardJson::boardDiscovered,this, &MainWindow::boardDiscovered);
    connect(jsonht, &BoardJson::statusMessage,this, &MainWindow::statusMessage);


    // Serial data ready
    connect(serialcon, &QSerialPort::readyRead,this, &MainWindow::serialReadReady);
    connect(serialcon, &QSerialPort::errorOccurred,this, &MainWindow::serialError);

    // Buttons
    connect(ui->cmdConnect,&QPushButton::clicked,this,&MainWindow::connectDisconnectClicked);
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
    connect(ui->chkResetDblTap, &QPushButton::clicked, this, &MainWindow::updateFromUI);

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
    connect(ui->spnRstDblTapMax, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnRstDblTapMin, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);
    connect(ui->spnRstDblTapThres, &QSpinBox::valueChanged, this, &MainWindow::updateFromUI);

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

    connect(ui->cmbpanchn,  &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbtiltchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbrllchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbalertchn, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
    connect(ui->cmbBtMode, &QComboBox::currentIndexChanged, this, &MainWindow::updateFromUI);
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

    boardDiscover = false;
    boardDiscoveryStarted = false;
    jsonht->disconnected();

    statusMessage(tr("Disconnected"));
    ui->cmdConnect->setText(tr(" Connect"));
    ui->cmdConnect->setIcon(QIcon(":/Icons/images/connect.svg"));
    ui->cmdChannelViewer->setEnabled(false);
    ui->cmdSaveNVM->setEnabled(false);
    ui->cmdReboot->setEnabled(false);
    ui->servoPan->setShowActualPosition(true);
    ui->servoTilt->setShowActualPosition(true);
    ui->servoRoll->setShowActualPosition(true);
    ui->cmdCalibrate->setEnabled(false);
    ui->cmdResetCenter->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
    ui->servoPan->setShowActualPosition(false);
    ui->servoTilt->setShowActualPosition(false);
    ui->servoRoll->setShowActualPosition(false);
    ui->actionEraseFlash->setEnabled(false);

    sending = false;
    connectTimer.stop();
    saveToRAMTimer.stop();
    requestTimer.stop();
    requestParamsTimer.stop();
    waitingOnParameters = false;
    waitingOnFeatures = false;
    serialData.clear();
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
    if(serialcon->isOpen()) {
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

        addToLog("GUI: " + sdata.mid(1,sdata.length()-6));
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
    log = log.trimmed();
    QString color = "black";
    if(ll==2) // Debug
        color = "red";
    else if(log.contains("<inf>"))
        color = "darkgreen";
    else if(log.contains("<err>"))
        color = "red";
    else if(log.contains("<wrn>"))
        color = "orange";
    else if(log.contains("<dbg>"))
        color = "DodgerBlue";
    else if(log.contains("GUI:"))
        color = "blue";
    else if(log.contains("\"Cmd\":\"Data\"")) // Don't show return measurment data
        return;
    log = log.toHtmlEscaped();

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
/*    if(trkset.getButtonPin() > 0)
        ui->chkLngBttnPress->setEnabled(true);
    else
        ui->chkLngBttnPress->setEnabled(false);
*/

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

    // Reset on double tap
    if(trkset.getRstOnDbltTap()) {
        ui->spnRstDblTapMax->setEnabled(true);
        ui->spnRstDblTapMin->setEnabled(true);
        ui->spnRstDblTapThres->setEnabled(true);
        ui->chkResetDblTap->setChecked(true);
    } else {
        ui->spnRstDblTapMax->setEnabled(false);
        ui->spnRstDblTapMin->setEnabled(false);
        ui->spnRstDblTapThres->setEnabled(false);
        ui->chkResetDblTap->setChecked(false);
    }
    ui->spnRstDblTapMax->setValue(trkset.getRstOnDblTapMax());
    ui->spnRstDblTapMin->setValue(trkset.getRstOnDblTapMin());
    ui->spnRstDblTapThres->setValue(trkset.getRstOnDblTapThres());

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

    ui->cmbBtMode->setCurrentIndex(trkset.getBtMode());
    int rot[3];
    trkset.orientation(rot[0],rot[1],rot[2]);
    ui->spnRotX->setValue(rot[0]);
    ui->spnRotY->setValue(rot[1]);
    ui->spnRotZ->setValue(rot[2]);

    ui->til_gain->setValue(trkset.getTlt_Gain()*10);
    ui->pan_gain->setValue(trkset.getPan_Gain()*10);
    ui->rll_gain->setValue(trkset.getRll_Gain()*10);

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

    // Button Press Mode - Enable/Disable on long press (Disable if no button pin selected)
    /*if(trkset.getButtonPin() > 0)
        ui->chkLngBttnPress->setEnabled(true);
    else
        ui->chkLngBttnPress->setEnabled(false);*/

    trkset.setButLngPs(ui->chkLngBttnPress->isChecked());
    trkset.setRstOnTlt(ui->chkRstOnTlt->isChecked());

    // Reset on Double Tap
    trkset.setRstOnDblTapMax(ui->spnRstDblTapMax->value());
    trkset.setRstOnDblTapMin(ui->spnRstDblTapMin->value());
    trkset.setRstOnDblTapThres(ui->spnRstDblTapThres->value());
    trkset.setRstOnDbltTap(ui->chkResetDblTap->isChecked());
    if(ui->chkResetDblTap->isChecked()) {
        ui->spnRstDblTapMax->setEnabled(true);
        ui->spnRstDblTapMin->setEnabled(true);
        ui->spnRstDblTapThres->setEnabled(true);
    } else {
        ui->spnRstDblTapMax->setEnabled(false);
        ui->spnRstDblTapMin->setEnabled(false);
        ui->spnRstDblTapThres->setEnabled(false);
    }

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
    if(ui->cmbBtMode->currentIndex() == TrackerSettings::BT_MODE_REMOTE ||
        ui->cmbBtMode->currentIndex() == TrackerSettings::BT_MODE_SCANNER)
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

        jsonht->dataIn(data);

        serialData = serialData.right(serialData.length()-nlindex-2);
    }
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
    if(!jsonht->isBoardSavedToRAM()) {
        QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not sent"),tr("Are you sure you want to disconnect?\n"\
                              "Changes haven't been sent to the headtracker\nClick \"Send Changes\" first"),QMessageBox::Yes|QMessageBox::No);
        if(rval != QMessageBox::Yes)
            return false;

    } else if (!jsonht->isBoardSavedToNVM()) {
        QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not saved on tracker"),tr("Are you sure you want to disconnect?\n"\
                              "Changes haven't been permanently stored on headtracker\nClick \"Save settings\" first"),QMessageBox::Yes|QMessageBox::No);
        if(rval != QMessageBox::Yes)
            return false;
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
 * it requests the hardware and version from the board
 */

void MainWindow::requestTimeout()
{
    if(boardDiscoveryStarted == false) {
        boardDiscoveryStarted = true;
        // Request hardware information from the new board
        addToLog(tr("Trying to connect to Head Tracker\n"));
        jsonht->requestHardware();
        requestTimer.start(WAIT_BETWEEN_BOARD_CONNECTIONS);
        return;
    }

    if(!boardDiscover && boardDiscoveryStarted) {
        msgbox->setText(tr("Was unable to determine the board type\n\nHave you written firmware to the board yet?"));
        msgbox->setWindowTitle("Error");
        msgbox->show();
        statusMessage(tr("Board discovery failed"));
        serialDisconnect();
    }
}

void MainWindow::saveToRAMTimeout()
{
    jsonht->saveToRAM();
}

void MainWindow::requestParamsTimeout()
{
    waitingOnParameters=true;
    jsonht->requestParameters();
}

/**
 * @brief MainWindow::eraseFlash This function will erase all configuration and reset to default
 */
void MainWindow::eraseFlash()
{
    if(QMessageBox::question(this, tr("Set Defaults?"), tr("This will erase all settings to defaults\r\nAre you sure?")) == QMessageBox::Yes) {
      jsonht->erase();
      jsonht->reboot();
      QTime dieTime= QTime::currentTime().addMSecs(RECONNECT_AFT_REBT);
      while (QTime::currentTime() < dieTime)
          QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

      serialConnect();
    }
}

// Start the various calibration dialogs
void MainWindow::startCalibration()
{
    if(!serialcon->isOpen()) {
        QMessageBox::information(this,tr("Not Connected"), tr("Connect before trying to calibrate"));
        return;
    }

    jsonht->startCalibration();
}

// Tell the board to start sending data again
void MainWindow::startData()
{
    jsonht->startData();
}

void MainWindow::storeToNVM()
{
    jsonht->saveToNVM();
    statusMessage(tr("Storing Parameters to memory"));
    ui->cmdSaveNVM->setEnabled(false);
}

void MainWindow::storeToRAM()
{
    jsonht->saveToRAM();
}

void MainWindow::resetCenter()
{
    jsonht->resetCenter();
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
    QMap<QString,bool> dataitms;
    if(jsonht->getFeatures().contains("IMU")) {
        dataitms["tiltoff"] = true;
        dataitms["rolloff"] = true;
        dataitms["panoff"] = true;
        dataitms["tiltout"] = true;
        dataitms["rollout"] = true;
        dataitms["panout"] = true;
    }
    dataitms["btcon"] = false;
    dataitms["gyrocal"] = false;
    dataitms["btaddr"] = false;
    dataitms["btrmt"] = false;

    switch(ui->tabBLE->currentIndex()) {
    case 0: { // General
        if(jsonht->getFeatures().contains("IMU"))
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
        if(jsonht->getFeatures().contains("BT")) {
            dataitms["btcon"] = true;
            dataitms["btrmt"] = true;
            dataitms["btaddr"] = true;
        }
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

void MainWindow::reboot()
{
    bool reboot=true;
    if(!jsonht->isBoardSavedToNVM()) {
        QMessageBox::StandardButton rval = QMessageBox::question(this,tr("Changes not saved"),tr("Are you sure you want to reboot?\n"\
                              "Changes haven't been saved\nClick \"Save Settings\" first"),QMessageBox::Yes|QMessageBox::No);
        if(rval != QMessageBox::Yes) {
            reboot = false;
        }
    }
    if(reboot) {
        jsonht->reboot();
        QTime dieTime= QTime::currentTime().addMSecs(RECONNECT_AFT_REBT);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        serialConnect();
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

    // Request the Features
    jsonht->requestFeatures();
    waitingOnFeatures = true;
}

void MainWindow::paramReceiveFailure(int)
{
    msgbox->setText(tr("Unable to receive the parameters"));
    msgbox->setWindowTitle("Error");
    msgbox->show();
    statusMessage("Parameter Received Failure");
    serialDisconnect();
}

void MainWindow::featuresReceiveStart()
{
    statusMessage(tr("Features Request Started"),5000);
}

void MainWindow::featuresReceiveComplete()
{
    statusMessage(tr("Features Request Complete"),5000);
    waitingOnFeatures = false;
    BLE33tabChanged(); // Request Data to be sent
    trkset.setDataItemSend("isCalibrated",true);
    trkset.setDataItemSend("trpenabled",true);
    QMap<QString, QVariant> pins = jsonht->getPins();
    QStringList features = jsonht->getFeatures();

    // Hide Everything
    ui->grpLocalGraph->setVisible(false);
    ui->grpRangeSel->setVisible(false);
    ui->lblGyroCal->setVisible(false);
    ui->gyroLed->setVisible(false);
    ui->cmdResetCenter->setVisible(false);
    ui->cmdResetCenter->setEnabled(true);
    ui->cmdCalibrate->setVisible(false);
    ui->cmdCalibrate->setEnabled(true);
    ui->cmdReboot->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(2);
    ui->cmdChannelViewer->setEnabled(true);
    ui->grpPPMInput->setVisible(false);
    ui->grpPPMOutput->setVisible(false);
    ui->cmdSaveNVM->setVisible(true);
    ui->cmdSaveNVM->setEnabled(true);
    ui->grpBoardRotation->setVisible(false);
    ui->chkLngBttnPress->setVisible(false);
    ui->chkResetCenterWave->setVisible(false);

    ui->tabPPM->setEnabled(false);
    ui->tabBT->setEnabled(false);
    ui->tabPWM->setEnabled(false);
    ui->tabAnaAux->setEnabled(false);
    ui->tabUart->setEnabled(false);

    // Show only the features the board has
    if(features.contains("IMU")) {
        ui->grpLocalGraph->setVisible(true);
        ui->grpRangeSel->setVisible(true);
        ui->lblGyroCal->setVisible(true);
        ui->gyroLed->setVisible(true);
        ui->cmdResetCenter->setVisible(true);
        ui->cmdCalibrate->setVisible(true);
        ui->grpBoardRotation->setVisible(true);
        ui->chkLngBttnPress->setVisible(true);
    }
    if(features.contains("PROXIMITY")) {
        ui->chkResetCenterWave->setVisible(true);
    }
    if(features.contains("CENTER")) {
        ui->lblCenterBtn->setText(pins["CENTER_BTN"].toString());
    } else {
        ui->lblCenterBtn->setText("No Center Button Defined");
    }
    if(features.contains("PPMIN")) {
        ui->grpPPMInput->setVisible(true);
        ui->lblPPMInputPin->setText(pins["PPMIN"].toString());
    }
    if(features.contains("PPMOUT")) {
        ui->grpPPMOutput->setVisible(true);
        ui->lblPPMOutputPin->setText(pins["PPMOUT"].toString());
    }
    if(features.contains("PPMIN") || features.contains("PPMOUT")) {
        ui->tabPPM->setEnabled(true);
    }
    if(features.contains("BT")) {
        ui->tabBT->setEnabled(true);
    }
    if(features.contains("PWM1CH") ||
       features.contains("PWM2CH") ||
       features.contains("PWM3CH") ||
       features.contains("PWM4CH")) {
        ui->tabPWM->setEnabled(true);
    }
    if(features.contains("AN1CH") ||
        features.contains("AN2CH") ||
        features.contains("AN3CH") ||
        features.contains("AN4CH")) {
        ui->tabAnaAux->setEnabled(true); // FixMe, Aux Shouldn't be hidden
    }
    if(features.contains("AUXSERIAL")) {
        ui->tabUart->setEnabled(true);
        if(features.contains("AUXINVERT")) {
            // TODO
        }
    }
    resize(100,100);
}


void MainWindow::featuresReceiveFailure(int)
{
    msgbox->setText(tr("Unable to receive the boards features"));
    msgbox->setWindowTitle("Error");
    msgbox->show();
    statusMessage("Features Receive Failure");
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
    sendSerialData(jsonht->dataout());
}

void MainWindow::needsCalibration()
{
    msgbox->setText(tr("Calibration has not been performed.\nPlease calibrate or restore from a saved file"));
    msgbox->setWindowTitle(tr("Calibrate"));
    msgbox->show();
}

void MainWindow::boardDiscovered()
{    
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
        msgbox->setText(tr("The firmware on the board is too old. Upload a ") + QString::number((float)lmajver/10,'f',1) + "x version of firmware for this GUI");
        msgbox->setWindowTitle(tr("Firmware Version Mismatch"));
        msgbox->show();
        serialDisconnect();

    // Firmware is too new
    } else if (lmajver < rmajver) {
        msgbox->setText(tr("Firmware is newer than supported by this application\nDownload the GUI v") + QString::number((float)rmajver/10,'f',1) +" from www.github.com/dlktdr/headtracker");
        msgbox->setWindowTitle(tr("Firmware Version Mismatch"));
        msgbox->show();
        serialDisconnect();
    }
    if(channelViewerOpen) {
        channelviewer->show();
    }
    boardDiscover = true;

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

