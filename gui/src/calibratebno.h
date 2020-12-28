#ifndef CalibrateBNO_H
#define CalibrateBNO_H

#include <QDialog>

namespace Ui {
class CalibrateBNO;
}

class CalibrateBNO : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrateBNO(QWidget *parent = nullptr);
    ~CalibrateBNO();
    bool calStatus;
public slots:
    void setCalibration(int sys, int mag, int gyr, int acc);
    void startCalibration();
signals:
    void calibrationComplete();
private:
    Ui::CalibrateBNO *ui;
    int sys,mag,gyr,acc;


};

#endif // CalibrateBNO_H
