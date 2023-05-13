#include "calibration.h"

Calibration::Calibration(QObject *parent)
    : QObject{parent}
{

}


void Calibration::openWizard(QWidget *parent = nullptr)
{
    if(wizard != nullptr)
        delete wizard;

    wizard = new QWizard(parent);



}

void Calibration::createPages()
{
    QWizardPage *accelPage = new QWizardPage();

    accelPage->setTitle(tr("Accelerometer"));
    wizard->addPage(accelPage);


}
