#include "calibratebno.h"
#include "ui_calibratebno.h"

CalibrateBNO::CalibrateBNO(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateBNO)
{
    ui->setupUi(this);
    sys=0;
    mag=0;
    gyr=0;
    acc=0;
    calStatus=false;
}

CalibrateBNO::~CalibrateBNO()
{
    delete ui;
}

void CalibrateBNO::setCalibration(int sys, int mag, int gyr, int acc)
{
    if(calStatus == true)
        return;

    QPixmap xpix(":/Icons/x.svg");
    QPixmap cpix(":/Icons/check.svg");
    int w = ui->sba->width() *2;
    int h = ui->sba->height() *2;
    xpix = xpix.scaled(w,h,Qt::KeepAspectRatio);
    cpix = cpix.scaled(w,h,Qt::KeepAspectRatio);

    ui->sba->setSignal(acc);
    ui->sbg->setSignal(gyr);
    ui->sbm->setSignal(mag);
    ui->sbs->setSignal(sys);

    if(acc == 3)
        ui->pxmAccel->setPixmap(cpix);
    else
        ui->pxmAccel->setPixmap(xpix);

    if(gyr == 3)
        ui->pxmGyro->setPixmap(cpix);
    else
        ui->pxmGyro->setPixmap(xpix);

    if(mag == 3)
        ui->pxmMag->setPixmap(cpix);
    else
        ui->pxmMag->setPixmap(xpix);

    if(sys == 3)
        ui->pxmSys->setPixmap(cpix);
    else
        ui->pxmSys->setPixmap(xpix);

    if(gyr < 3) {
        ui->lblStep->setText("Gyro Calibration\n\nLeave device sitting stable");
    } else if (acc < 3) {
        ui->lblStep->setText("Accelerometer Calibration\n\nRotate in various 90 degree orientations. Hold device stable for 4 seconds.");
    } else if (mag < 3) {
        ui->lblStep->setText("Magnetometer Calibration\n\nRotate in a figure 8 pattern");
    } else if (sys < 3) {
        ui->lblStep->setText("System Calibration\n\nRotate device in a 360 and place tracker down where stable");
    } else {
        ui->lblStep->setText("Calibration Success!");
        calStatus = true;
    }
}

void CalibrateBNO::startCalibration()
{
    calStatus = false;
}
