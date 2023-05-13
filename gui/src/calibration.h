#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QObject>
#include <QWizard>

class Calibration : public QObject
{
    Q_OBJECT
public:
    explicit Calibration(QObject *parent = nullptr);
public slots:
    void openWizard(QWidget *parent);
signals:
protected:
    QWizard *wizard = nullptr;
    void createPages();
};

#endif // CALIBRATION_H
