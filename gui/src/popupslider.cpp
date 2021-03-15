#include <QApplication>
#include <QPainterPath>
#include "popupslider.h"

FSpinBox::FSpinBox(QWidget *parent) : QSpinBox(parent)
{
    setKeyboardTracking(false);
}

void FSpinBox::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if(QApplication::focusWidget() != this->parentWidget()) {
        hide();
        this->parentWidget()->hide();
    }

}

PopupSlider::PopupSlider(QWidget *parent) : QWidget(parent)
{
    padding = 10;
    widvertoff = 10;
    spinbox = new FSpinBox(this);
    resize(90,30 + padding + widvertoff);
    spinbox->move(padding/2,padding/2);
    spinbox->resize(width()-padding,height()-padding-widvertoff);
    spinbox->setSuffix(" uS");
    spinbox->setAlignment(Qt::AlignCenter);
    connect(spinbox, SIGNAL(valueChanged(int)), this, SLOT(valueCh(int)));
    setFocusPolicy(Qt::ClickFocus);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
}

void PopupSlider::setLimits(int low, int high)
{
    spinbox->setMaximum(high);
    spinbox->setMinimum(low);
}

void PopupSlider::setValue(int val)
{
    spinbox->setValue(val);
    spinbox->setFocusPolicy(Qt::StrongFocus);
    spinbox->setFocus();
    spinbox->selectAll();
}

void PopupSlider::setSuffix(QString suf)
{
    spinbox->setSuffix(suf);
}

void PopupSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    int slwi = 8; // Slider width
    slwi /= 2;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setRenderHint(QPainter::HighQualityAntialiasing);
    QPainterPath path;
    path.moveTo(0,0);
    path.lineTo(width(),0);
    path.lineTo(width(),height()-widvertoff);
    path.lineTo((width()) - ((width())*.4-slwi),height()-widvertoff);
    path.lineTo((width())/2+slwi,height());
    path.lineTo((width())/2-slwi,height());
    path.lineTo(((width())*.4-slwi),height()-widvertoff);
    path.lineTo(0,height()-widvertoff);
    path.closeSubpath();

    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::black);
    painter.drawPath(path);
}

void PopupSlider::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if(QApplication::focusWidget() != spinbox)
        hide();
}

void PopupSlider::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    spinbox->show();
}

void PopupSlider::valueCh(int a)
{
    emit valueChanged(a);
}

