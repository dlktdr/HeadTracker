#include "calaccelerometer.h"
#include "ui_calaccelerometer.h"

CalAccelerometer::CalAccelerometer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalAccelerometer)
{
    ui->setupUi(this);
}

CalAccelerometer::~CalAccelerometer()
{
    delete ui;
}

void CalAccelerometer::rawAccelChanged(float x, float y, float z)
{
    float beta=ACCEL_FILTER_BETA;
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

