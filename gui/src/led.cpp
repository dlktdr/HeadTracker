#include "led.h"

Led::Led(QWidget *parent) : QWidget(parent)
{    
    blink = false;
    state = false;
    blinkstate = false;
    colon = Qt::green;
    coloff = Qt::red;
    colblink = QColor(251, 255, 0);
    color = coloff;
    connect(&blinktmr,SIGNAL(timeout()),this,SLOT(blinkTimeout()));
    blinktmr.setInterval(250);
    blinktmr.start();
}

void Led::blinkTimeout()
{
    // If state on, never blink
    if(state) {
        color = colon;
        return;
    }

    // If state off blink
    blinkstate = !blinkstate;
    if(blink && blinkstate)
        color = colblink;
    else
        color = coloff;
    update();
}


void Led::setState(bool a)
{
    state = a;
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
    gm.setColorAt(1,color.darker(120));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(QBrush(gm));
    painter.setPen(QPen(Qt::black));
    painter.drawEllipse(1,1,width()-2,height()-2);
}
