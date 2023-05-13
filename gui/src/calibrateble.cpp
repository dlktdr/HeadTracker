#include "calibrateble.h"
#include "ui_calibrateble.h"

CalibrateBLE::CalibrateBLE(TrackerSettings *ts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateBLE),
    trkset(ts)
{
    ui->setupUi(this);
    step = 0;

    connect(ui->cmdNext,SIGNAL(clicked()),this,SLOT(nextClicked()));
    connect(ui->cmdPrevious,SIGNAL(clicked()),this,SLOT(prevClicked()));

    ui->stackedWidget->setCurrentIndex(0);

    ui->magcalwid->setTracker(trkset);
    connect(ui->magcalwid,&MagCalWidget::dataUpdate,this,&CalibrateBLE::dataUpdate);

    // ACCEL
    accStep = 0;
    for(int i=0; i < 3; i++) {
        _accInverted[i] = false;
        _accOff[i] = 0;
        _currentAccel[i] = 0;
    }

    connect(trkset,&TrackerSettings::rawAccelChanged,this, &CalibrateBLE::rawAccelChanged);
    connect(ui->cmdAccNext, &QPushButton::clicked, this, &CalibrateBLE::setNextAccStep);
    connect(ui->cmdRestart, &QPushButton::clicked, this, &CalibrateBLE::restartCal);
    connect(ui->chkUseMagnetometer, &QCheckBox::toggled, this, &CalibrateBLE::useMagToggle);
}

CalibrateBLE::~CalibrateBLE()
{
    delete ui;
}

void CalibrateBLE::setNextAccStep()
{
    switch(accStep) {
    case ZP:
        ui->ledZMax->setBlink(false);
        ui->ledZMax->setState(true);
        ui->ledZMin->setBlink(true);

        if(_currentAccel[2] < 0)
            _accInverted[2] = true;

        _accFirst = _currentAccel[2];        
        accStep++;
        break;
    case ZM:
        ui->ledZMin->setBlink(false);
        ui->ledZMin->setState(true);
        ui->ledYMax->setBlink(true);
        _accOff[2] = fabs(_accFirst + _currentAccel[2]) / 2.0f;
        if(_accInverted[2])
            _accOff[2] *= -1.0f;
        qDebug() << "OFFSET " << _accOff[2];
        accStep++;
        break;
    case YP:
        ui->ledYMax->setBlink(false);
        ui->ledYMax->setState(true);
        ui->ledYMin->setBlink(true);

        if(_currentAccel[1] < 0)
            _accInverted[1] = true;
        _accFirst = _currentAccel[1];

        accStep++;
        break;
    case YM:
        ui->ledYMin->setBlink(false);
        ui->ledYMin->setState(true);
        ui->ledXMax->setBlink(true);
        _accOff[1] = fabs(_accFirst + _currentAccel[1]) / 2.0f;
        if(_accInverted[1])
            _accOff[1] *= -1.0f;
        qDebug() << "OFFSET " << _accOff[1];
        accStep++;
        break;
    case XP:
        ui->ledXMax->setBlink(false);
        ui->ledXMax->setState(true);
        ui->ledXMin->setBlink(true);

        if(_currentAccel[0] < 0)
            _accInverted[0] = true;
        _accFirst = _currentAccel[0];

        accStep++;
        break;
    case XM:
        ui->ledXMin->setBlink(false);
        ui->ledXMin->setState(true);
        ui->cmdNext->setDisabled(false);
        _accOff[0] = fabs(_accFirst + _currentAccel[0]) / 2.0f;
        if(_accInverted[0])
            _accOff[0] *= -1.0f;
        qDebug() << "OFFSET " << _accOff[0];
        accStep++;
        // COMPLETE
        break;
    }
    ui->accelImage->setPixmap(QPixmap(accel_cal_images[accStep]));
    if(accStep == ACCCOMPLETE) {
        step=STEP_END;
        setButtonText();
        ui->accelImage->setPixmap(QPixmap());
        ui->accelImage->setText(tr("Accelerometer Calibration Complete"));
    }
}

void CalibrateBLE::restartCal()
{
    ui->ledXMax->setState(false);
    ui->ledXMin->setState(false);
    ui->ledYMax->setState(false);
    ui->ledYMin->setState(false);
    ui->ledZMax->setState(false);
    ui->ledZMin->setState(false);

    ui->ledXMax->setBlink(false);
    ui->ledXMin->setBlink(false);
    ui->ledYMax->setBlink(false);
    ui->ledYMin->setBlink(false);
    ui->ledZMax->setBlink(true);
    ui->ledZMin->setBlink(false);
    accStep = 0;
    ui->accelImage->setPixmap(QPixmap(accel_cal_images[accStep]));
    ui->accelImage->setText("");
}

void CalibrateBLE::useMagToggle()
{
    trkset->setDisMag(!ui->chkUseMagnetometer->isChecked());
    emit saveToRam();
}

void CalibrateBLE::setButtonText()
{
    if(step == 0) {
        ui->cmdPrevious->setText("Cancel");
    } else {
        ui->cmdPrevious->setText("Back");
    }
    if(step < STEP_END) {
        ui->cmdNext->setText("Next");
    } else {
        ui->cmdNext->setText("Save");
    }
    ui->cmdPrevious->setDisabled(false);
    ui->cmdNext->setDisabled(false);
}

void CalibrateBLE::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    ui->chkUseMagnetometer->setChecked(!trkset->getDisMag());
}

void CalibrateBLE::nextClicked()
{
    if(step == STEP_MAGINTRO) {
        trkset->setDisMag(!ui->chkUseMagnetometer->isChecked());
    }
    step++;
    if(step < STEP_END) {
        ui->stackedWidget->setCurrentIndex(step);
    } else {
        trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
        trkset->setSoftIronOffsets(_soo);
        trkset->setAccOffset(_accOff[0], _accOff[1], _accOff[2]);
        emit calibrationSave();
        step = 0;
        ui->stackedWidget->setCurrentIndex(step);
        hide();
    }
    setButtonText();
    if(step == STEP_MAGCAL) {
        _hoo[0] = 0;
        _hoo[1] = 0;
        _hoo[2] = 0;
        for(int i=0; i<3; i++)
            for(int j=0; j<3; j++)
                _soo[i][j] = 0;
        ui->cmdNext->setDisabled(true);
        ui->magcalwid->resetDataPoints();
        if(trkset->getDisMag()) // Skip this step if mag disabled
            nextClicked();
    } else if (step == STEP_ACCELCAL) {
        restartCal();
        ui->cmdNext->setDisabled(true);
    } else if( step == STEP_END) {

    }
}

void CalibrateBLE::prevClicked()
{
    if(step == 0) {
        emit calibrationCancel();
        hide();
    }
    if(step == STEP_END)
        step--; // Jump back two, for buttons
    if(step > 0)
        step--;
    setButtonText();
    ui->stackedWidget->setCurrentIndex(step);

    if(step == STEP_MAGCAL) {
        ui->cmdNext->setDisabled(true);
        trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
        trkset->setSoftIronOffsets(_soo);
        ui->magcalwid->resetDataPoints();
        if(trkset->getDisMag()) // Skip this step if mag disabled
            prevClicked();
    } else if (step == STEP_ACCELCAL) {
        restartCal();
        ui->cmdNext->setDisabled(true);
        trkset->setAccOffset(_accOff[0], _accOff[1], _accOff[2]);
    }
}

void CalibrateBLE::dataUpdate(float variance,
                              float gaps,
                              float wobble,
                              float fiterror,
                              float hoo[3],
                              float soo[3][3])
{
    ui->lblFitError->setText(QString::number(fiterror,'g',3) + "%");
    ui->lblVariance->setText(QString::number(variance,'g',3) + "%");
    ui->lblWobble->setText(QString::number(wobble,'g',3) + "%");
    ui->lblGaps->setText(QString::number(gaps,'g',3) + "%");

    // On the magcal step and values all good?
    if((step == STEP_MAGCAL && gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f) ||
        step == STEP_ACCELCAL)
        ui->cmdNext->setDisabled(false);
    else
        ui->cmdNext->setDisabled(true);

    // Save the current offsets locally
    for(int i=0;i<3;i++) {
        for(int j=0;j<3;j++) {
            _soo[i][j] = soo[i][j];
        }
        _hoo[i] = hoo[i];
    }
}

void CalibrateBLE::rawAccelChanged(float x, float y, float z)
{
    float beta=0.3f;
    _currentAccel[0] = ((1.0f - beta) * _currentAccel[0]) + (beta * x);
    _currentAccel[1] = ((1.0f - beta) * _currentAccel[1]) + (beta * y);
    _currentAccel[2] = ((1.0f - beta) * _currentAccel[2]) + (beta * z);
    qDebug() << "ACCEL " << _currentAccel[0] << _currentAccel[1] << _currentAccel[2];

    switch(accStep) {
    case ZP:
        ui->lblCurVal->setText(QString::number(_currentAccel[2],'f',2));
        break;
    case ZM:
        ui->lblCurVal->setText(QString::number(_currentAccel[2],'f',2));
        break;
    case YP:
        ui->lblCurVal->setText(QString::number(_currentAccel[1],'f',2));
        break;
    case YM:
        ui->lblCurVal->setText(QString::number(_currentAccel[1],'f',2));
        break;
    case XP:
        ui->lblCurVal->setText(QString::number(_currentAccel[0],'f',2));
        break;
    case XM:
        ui->lblCurVal->setText(QString::number(_currentAccel[0],'f',2));
        break;

    }
}
