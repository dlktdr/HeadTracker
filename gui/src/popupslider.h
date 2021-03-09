#ifndef PopupSlider_H
#define PopupSlider_H

#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QStyleOptionFrame>
#include <QSpinBox>

class FSpinBox : public QSpinBox
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
    void setLimits(int low, int high);
    void setValue(int val);
    void setSuffix(QString);

signals:
    void valueChanged(int value);

private:
    FSpinBox *spinbox;

    void paintEvent(QPaintEvent *event);
    void focusOutEvent(QFocusEvent* event);
    void showEvent(QShowEvent *event);
    int padding;
    int widvertoff;

private slots:
    void valueCh(int a);
};





#endif // PopupSlider_H
