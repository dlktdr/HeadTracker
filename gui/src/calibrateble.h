#ifndef CALIBRATEBLE_H
#define CALIBRATEBLE_H

#include <QDialog>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include "trackersettings.h"

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

    double mag[3];
    double magmin[3];
    double magmax[3];
    double magoff[3];
    bool firstmag=true;

private slots:
    void rawGyroChanged(float x, float y, float z);
    void rawMagChanged(float x, float y, float z);
    void nextClicked();
    void prevClicked();
signals:
    void calibrationSave();
};

#endif // CALIBRATEBLE_H
