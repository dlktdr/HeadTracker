#include "led.h"

Led::Led(QWidget *parent) : QWidget(parent)
{    
    colon =Qt::green;
    coloff = Qt::red;
    color = coloff;
}

void Led::setState(bool a)
{
    if(a)
        color = colon;
    else
        color = coloff;
    update();
}

void Led::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    QRadialGradient gm(width()/2,height()/2,width()/2);
    gm.setColorAt(0,color);
    gm.setColorAt(1,color.darker(160));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(QBrush(gm));
    painter.setPen(QPen(Qt::black));
    painter.drawEllipse(1,1,width()-2,height()-2);
}
