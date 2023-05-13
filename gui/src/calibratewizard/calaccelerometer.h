#ifndef CALACCELEROMETER_H
#define CALACCELEROMETER_H

#include <QWidget>

#define ACCEL_FILTER_BETA 0.3f
constexpr char accel_cal_images[6][35] = {":/Icons/images/components_up.png",
                                          ":/Icons/images/components_down.png",
                                          ":/Icons/images/edge_right.png",
                                          ":/Icons/images/edge_left.png",
                                          ":/Icons/images/USB_up.png",
                                          ":/Icons/images/USB_down.png"};

namespace Ui {
class CalAccelerometer;
}

class CalAccelerometer : public QWidget
{
    Q_OBJECT

public:
    explicit CalAccelerometer(QWidget *parent = nullptr);
    ~CalAccelerometer();

public slots:
    void rawAccelChanged(float x, float y, float z);

private:
    Ui::CalAccelerometer *ui;
    float _accOff[3];
    float _accZ[2]; // Accelerometer Z Min & Max
    float _accY[2]; // Accelerometer Z Min & Max
    float _accX[2]; // Accelerometer Z Min & Max
    float _accFirst;
    float _currentAccel[3];
    bool _accInverted[3];
    enum ACCSTEP {ZP,ZM,YP,YM,XP,XM,ACCCOMPLETE};
    int accStep;
};

#endif // CALACCELEROMETER_H
