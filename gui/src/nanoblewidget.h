#ifndef NANOBLEWIDGET_H
#define NANOBLEWIDGET_H

#include <QWidget>

namespace Ui {
class NanoBLEWidget;
}

class NanoBLEWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NanoBLEWidget(QWidget *parent = nullptr);
    ~NanoBLEWidget();

private:
    Ui::NanoBLEWidget *ui;
};

#endif // NANOBLEWIDGET_H
