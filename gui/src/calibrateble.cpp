#include <math.h>

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
    connect(trkset,&TrackerSettings::rawMagChanged,this,&CalibrateBLE::rawMagChanged);

    connect(ui->cmdNext,SIGNAL(clicked()),this,SLOT(nextClicked()));
    connect(ui->cmdPrevious,SIGNAL(clicked()),this,SLOT(prevClicked()));
    ui->stackedWidget->setCurrentIndex(0);
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

void CalibrateBLE::rawMagChanged(float x, float y, float z)
{
    // *** To do - fit to elipsode Maybe some 3d graphing

    ui->lblMagX->setText(QString::number(x,'g',3));
    ui->lblMagY->setText(QString::number(y,'g',3));
    ui->lblMagZ->setText(QString::number(z,'g',3));
    mag[0] = x;
    mag[1] = y;
    mag[2] = z;
    // On first run set min/max to cur val
    if(firstmag) {
        for(int i=0;i<3;i++) {
            magmax[i] = mag[i];
            magmin[i] = mag[i];
        }
        firstmag = false;
    }

    for(int i=0;i<3;i++) {
        magmax[i] = MAX(mag[i],magmax[i]);
        magmin[i] = MIN(mag[i],magmin[i]);
        magoff[i] = (magmax[i]+magmin[i])/2;
    }

    ui->lblMagXMax->setText(QString::number(magmax[0],'g',3));
    ui->lblMagYMax->setText(QString::number(magmax[1],'g',3));
    ui->lblMagZMax->setText(QString::number(magmax[2],'g',3));
    ui->lblMagXMin->setText(QString::number(magmin[0],'g',3));
    ui->lblMagYMin->setText(QString::number(magmin[1],'g',3));
    ui->lblMagZMin->setText(QString::number(magmin[2],'g',3));
    ui->lblMagXOff->setText(QString::number(magoff[0],'g',3));
    ui->lblMagYOff->setText(QString::number(magoff[1],'g',3));
    ui->lblMagZOff->setText(QString::number(magoff[2],'g',3));
}


void CalibrateBLE::nextClicked()
{
    switch (step) {
    // Gyrometer
    case 0: {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cmdPrevious->setText("Previous");

        firstmag = true;
        for(int i=0;i<3;i++)
            gyrsaveoff[i] = gyroff[i]; // Values to save
        step = 1;
        break;
    }
    // Magnetometer
    case 1: {
        ui->stackedWidget->setCurrentIndex(2);
        ui->cmdNext->setText("Save");
        step = 2;
        break;
    }
    // Accelerometer - If clicked here save + close
    case 2: {
        // SAVE HERE + CLOSE
        hide();
        ui->stackedWidget->setCurrentIndex(0);
        ui->cmdNext->setText("Next");
        trkset->setMagOffset(roundf(magoff[0]*1000)/1000,
                             roundf(magoff[1]*1000)/10000,
                             roundf(magoff[2]*1000)/1000);
        trkset->setGyroOffset(roundf(gyrsaveoff[0]*1000)/1000,
                              roundf(gyrsaveoff[1]*1000)/1000,
                              roundf(gyrsaveoff[2]*1000)/1000);
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
