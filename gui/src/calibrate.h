#ifndef CALIBRATE_H
#define CALIBRATE_H

#include <QDialog>

namespace Ui {
class Calibrate;
}

class Calibrate : public QDialog
{
    Q_OBJECT

public:
    explicit Calibrate(QWidget *parent = nullptr);
    ~Calibrate();
    bool calStatus;
public slots:
    void setCalibration(int sys, int mag, int gyr, int acc);
    void startCalibration();
signals:
    void calibrationComplete();
private:
    Ui::Calibrate *ui;
    int sys,mag,gyr,acc;


};

#endif // CALIBRATE_H
