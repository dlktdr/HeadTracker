#ifndef PopupSlider_H
#define PopupSlider_H

#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QStyleOptionFrame>
#include <QDoubleSpinBox>

class FSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit FSpinBox(QWidget *parent=nullptr);
protected:
    void focusOutEvent(QFocusEvent* event);
};

class PopupSlider : public QWidget
{
    Q_OBJECT
public:
    explicit PopupSlider(QWidget *parent = nullptr);

public slots:
    void setLimits(double low, double high);
    void setValue(double val);
    void setSuffix(QString);
    void setPrecision(int dp);
    void setStep(double v);

signals:
    void valueChanged(double value);

private:
    FSpinBox *spinbox;

    void paintEvent(QPaintEvent *event);
    void focusOutEvent(QFocusEvent* event);
    void showEvent(QShowEvent *event);
    int padding;
    int widvertoff;

private slots:
    void valueCh(double a);
};





#endif // PopupSlider_H
