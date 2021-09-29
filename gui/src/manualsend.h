#ifndef MANUALSEND_H
#define MANUALSEND_H

#include <QWidget>
#include <QtSerialPort>
#include "trackersettings.h"

namespace Ui {
class ManualSend;
}

class ManualSend : public QWidget
{
    Q_OBJECT

public:
    explicit ManualSend(QWidget *parent, TrackerSettings *ts);
    ~ManualSend();
    void setSerialPort(QSerialPort *p) {port = p;}

private:
    Ui::ManualSend *ui;
    TrackerSettings *trkset;
    QSerialPort *port = nullptr;
private slots:
    void sendClicked();
    void commandChange();
};

#endif // MANUALSEND_H
