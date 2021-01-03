#include "calibrateble.h"
#include "ui_calibrateble.h"

CalibrateBLE::CalibrateBLE(TrackerSettings *ts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateBLE),
    trkset(ts)
{
    ui->setupUi(this);
    step = 0;

    connect(trkset,&TrackerSettings::rawGyroChanged,this,&CalibrateBLE::rawGyroChanged);    

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

void CalibrateBLE::rawGyroChanged(float x, float y, float z)
{
    // Raw Values
    ui->lblGyroX->setText(QString::number(x,'g',3));
    ui->lblGyroY->setText(QString::number(y,'g',3));
    ui->lblGyroZ->setText(QString::number(z,'g',3));

    // Moving average
    double a = MOVING_AVERAGE;
    gyroff[0] = a*x + (gyroff[0]*(1-a));
    gyroff[1] = a*y + (gyroff[1]*(1-a));
    gyroff[2] = a*z + (gyroff[2]*(1-a));

    // Display the values
    ui->lblGyroXOff->setText(QString::number(gyroff[0],'g',2));
    ui->lblGyroYOff->setText(QString::number(gyroff[1],'g',2));
    ui->lblGyroZOff->setText(QString::number(gyroff[2],'g',2));

    /*qint64 curtime = QDateTime::currentMSecsSinceEpoch();
    double period = (double)(curtime - lasttime)/ 1000.0;
    lasttime = curtime;
    qDebug() << "Period" << period;
    if(period == 0) {
        period = 0.1;
    }

    // Rate of change on average
    for(int i=0;i<3;i++) {
        gyrate[i] += 0.015 * (gyrate[i] - lgyrate[i]);   // Low pass
        lgyrate[i] = gyrate[i];

        gyrate[i] = (gyroff[i] - lgyroff[i]) / period; // Differentiate
        lgyroff[i] = gyroff[i];

        qDebug() << "Gyro Rate" << i << gyrate[i];
    }

    bool done=true;
    if(abs(gyrate[0]) > 0.1) {
        ui->lblGyroXOff->setStyleSheet("background-color: red;");
        done = false;
    } else
        ui->lblGyroXOff->setStyleSheet("background-color: rgb(0, 229, 0);");

    if(abs(gyrate[1]) > 0.1) {
        ui->lblGyroYOff->setStyleSheet("background-color: red;");
        done = false;
    } else
        ui->lblGyroYOff->setStyleSheet("background-color: rgb(0, 229, 0);");

    if(abs(gyrate[2]) > 0.1) {
        ui->lblGyroZOff->setStyleSheet("background-color: red;");
        done = false;
    } else
        ui->lblGyroZOff->setStyleSheet("background-color: rgb(0, 229, 0);");


    if(done) {
        enableNext.start(1000);
    } else {
        ui->cmdNext->setEnabled(false);
        enableNext.stop();
    }*/

}


void CalibrateBLE::nextClicked()
{
    switch (step) {
    // Gyrometer
    case 0: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdPrevious->setText("Previous");

        // REMOVE ONCE ACCEL CAL READY
           ui->cmdNext->setText("Save");
        // ------------

        firstmag = true;
        for(int i=0;i<3;i++)
            gyrsaveoff[i] = gyroff[i]; // Values to save
        step = 1;
      //  ui->cmdNext->setDisabled(true);
        break;
    }
    // Magnetometer
    case 1: {
        ui->stackedWidget->setCurrentIndex(2);
        ui->cmdNext->setText("Save");
         step = 2;

        // REMOVE ONCE ACCEL CAL READY
            hide();
            ui->stackedWidget->setCurrentIndex(0);
            ui->cmdNext->setText("Next");
            trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
            trkset->setSoftIronOffsets(_soo);
            trkset->setGyroOffset(gyrsaveoff[0],gyrsaveoff[1],gyrsaveoff[2]);
            emit calibrationSave();
            step = 0;
        //----------------------------------------



        break;
    }
    // Accelerometer - If clicked here save + close
    case 2: {
        // SAVE HERE + CLOSE
        hide();
        ui->stackedWidget->setCurrentIndex(0);
        ui->cmdNext->setText("Next");
        trkset->setMagOffset(_hoo[0], _hoo[1], _hoo[2]);
        trkset->setSoftIronOffsets(_soo);
        trkset->setGyroOffset(gyrsaveoff[0],gyrsaveoff[1],gyrsaveoff[2]);
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
    case 0: {
        step = 0;

        hide();
        break;
    }
    // Magnetometer
    case 1: {
        ui->cmdPrevious->setText("Cancel");
        ui->cmdNext->setText("Next");
        ui->stackedWidget->setCurrentIndex(0);
        step = 0;
        break;
    }
    // Accelerometer
    case 2: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdNext->setText("Next");
        step = 1;
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

    if(step == 1) { // On the magcal step?
        if(gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f)
            ui->cmdNext->setDisabled(false);
        else if(gaps > 20.0f && variance > 5.0f && wobble > 5.0f && fiterror > 6.0f)
            ui->cmdNext->setDisabled(true);
    }

    // Save the current offsets locally
    for(int i=0;i<3;i++) {
        for(int j=0;j<3;j++) {
            _soo[i][j] = soo[i][j];
        }
        _hoo[i] = hoo[i];
    }
}
