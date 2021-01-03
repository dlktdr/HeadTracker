#ifndef CALIBRATEBLE_H
#define CALIBRATEBLE_H

#include <QDialog>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include "trackersettings.h"
#include "magcalwidget.h"

#define MIN(x,y) x < y ? x : y
#define MAX(x,y) x > y ? x : y

namespace Ui {
class CalibrateBLE;
}

class CalibrateBLE : public QDialog
{
    Q_OBJECT
public:
    explicit CalibrateBLE(TrackerSettings *ts, QWidget *parent = nullptr);
    ~CalibrateBLE();

private:
    static constexpr double MOVING_AVERAGE=0.03;
    Ui::CalibrateBLE *ui;
    TrackerSettings *trkset;
    int step;
    double gyr[3];
    double gyroff[3];
    double lgyroff[3];
    double gyrate[3];
    double lgyrate[3];
    double gyrsaveoff[3];

    float _soo[3][3]; // Soft Iron
    float _hoo[3]; // Hard Iron
    bool firstmag=true;    

private slots:
    void rawGyroChanged(float x, float y, float z);
    void nextClicked();
    void prevClicked();
    void dataUpdate(float variance,
                    float gaps,
                    float wobble,
                    float fiterror,
                    float hoop[3],
                    float soo[3][3]);
signals:
    void calibrationSave();
};

#endif // CALIBRATEBLE_H
