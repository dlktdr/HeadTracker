#include "nanoblewidget.h"
#include "ui_nanoblewidget.h"

NanoBLEWidget::NanoBLEWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NanoBLEWidget)
{
    ui->setupUi(this);
}

NanoBLEWidget::~NanoBLEWidget()
{
    delete ui;
}
