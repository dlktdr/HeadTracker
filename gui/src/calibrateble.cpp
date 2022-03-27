#include "calibrateble.h"
#include "ui_calibrateble.h"

CalibrateBLE::CalibrateBLE(TrackerSettings *ts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateBLE),
    trkset(ts)
{
    ui->setupUi(this);
    step = 0;

//    connect(trkset,&TrackerSettings::rawGyroChanged,this,&CalibrateBLE::rawGyroChanged);

    connect(ui->cmdNext,SIGNAL(clicked()),this,SLOT(nextClicked()));
    connect(ui->cmdPrevious,SIGNAL(clicked()),this,SLOT(prevClicked()));

    ui->stackedWidget->setCurrentIndex(0);

    ui->magcalwid->setTracker(trkset);

    connect(ui->magcalwid,&MagCalWidget::dataUpdate,this,&CalibrateBLE::dataUpdate);
}

CalibrateBLE::~CalibrateBLE()
{
    delete ui;
}



void CalibrateBLE::nextClicked()
{
    switch (step) {
    // Gyrometer
    /*case GYROCAL: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdPrevious->setText("Previous");

        // REMOVE ONCE ACCEL CAL READY
           ui->cmdNext->setText("Save");
        // ------------

        firstmag = true;
        for(int i=0;i<3;i++)
            gyrsaveoff[i] = gyroff[i]; // Values to save
        step = MAGCAL;
        ui->cmdNext->setDisabled(true);
        ui->magcalwid->resetDataPoints();
        break;
    }*/
    // Magnetometer
    case MAGCAL: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdNext->setText("Save");
         step = ACCELCAL;

        // REMOVE ONCE ACCEL CAL READY
            hide();
            ui->stackedWidget->setCurrentIndex(0);
            ui->cmdNext->setText("Next");
            trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
            trkset->setSoftIronOffsets(_soo);
//            trkset->setGyroOffset(gyrsaveoff[0],gyrsaveoff[1],gyrsaveoff[2]);
            emit calibrationSave();
            step = MAGCAL;
        //----------------------------------------

        break;
    }
    // Accelerometer - If clicked here save + close
    case ACCELCAL: {
        // SAVE HERE + CLOSE
        hide();
        ui->stackedWidget->setCurrentIndex(0);
        ui->cmdNext->setText("Next");
        trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
        trkset->setSoftIronOffsets(_soo);
        //trkset->setGyroOffset(gyrsaveoff[0],gyrsaveoff[1],gyrsaveoff[2]);
        emit calibrationSave();
        step = 0;
        break;
    }
    }
}

void CalibrateBLE::prevClicked()
{
    switch (step) {
    // Gyrometer - If clicked here close
/*    case GYROCAL: {
        emit calibrationCancel();
        hide();
        break;
    }*/
    // Magnetometer
    case MAGCAL: {
        ui->cmdPrevious->setText("Cancel");
        ui->cmdNext->setText("Complete");
        ui->cmdNext->setDisabled(false);
        ui->stackedWidget->setCurrentIndex(0);
        // Cancel Clicked, start over
        ui->magcalwid->resetDataPoints();
        emit calibrationCancel();
        hide();
        step = MAGCAL;
        break;
    }
    // Accelerometer
    case ACCELCAL: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdNext->setText("Next");
        step = MAGCAL;
        firstmag = true;
        break;
    }
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
    if(step == MAGCAL && gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f)
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
