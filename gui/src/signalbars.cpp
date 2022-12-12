#include "signalbars.h"

SignalBars::SignalBars(QWidget *parent) : QWidget(parent)
{
    signal = 3;
}

void SignalBars::paintEvent(QPaintEvent *event)
{

    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    Q_UNUSED(event);
    QPainter painter(this);

    QLinearGradient gm(0,0,0,height());
    gm.setColorAt(0,Qt::green);
    gm.setColorAt(1,QColor(Qt::green).darker(170));

    QStyleOptionFrame sof;

    QBrush brushon(gm);
    QBrush brushoff(Qt::transparent);
    QPen pen(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brushoff);
    qreal barwidth = (width()-1)/4;
    qreal barspace = (width()-1)/8;
    qreal barheight = (height()-1)/3;

    if(signal > 2)
        painter.setBrush(brushon);
    painter.drawRect(barwidth*2+barspace*2, 0,                  barwidth,   3*barheight);

    if(signal > 1)
        painter.setBrush(brushon);
    painter.drawRect(barwidth+barspace,     barheight*1,        barwidth,   2*barheight);

    if(signal > 0)
        painter.setBrush(brushon);
    painter.drawRect(1,                     barheight*2,        barwidth,   barheight);
}
