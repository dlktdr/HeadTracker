#include "gainslider.h"

GainSlider::GainSlider(QWidget *parent) : QSlider(parent)
{
    popslide = new PopupSlider(this);
    popslide->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    popslide->setSuffix("");

    connect(this,&QSlider::sliderPressed,this,&GainSlider::sliderPressedH);
    connect(this,&QSlider::sliderMoved,this,&GainSlider::sliderMovedH);
    connect(popslide,&PopupSlider::valueChanged,this,&GainSlider::popValChanged);
}

GainSlider::~GainSlider()
{
    delete popslide;
}

void GainSlider::setSliderPosition(int p)
{
    QSlider::setSliderPosition(p);
}

void GainSlider::setMaximum(int val)
{
    QSlider::setMaximum(val);
    popslide->setLimits(minimum(),maximum());
}

void GainSlider::setMinimum(int val)
{
    QSlider::setMinimum(val);
    popslide->setLimits(minimum(),maximum());
}

void GainSlider::setValue(int val)
{
    //popslide->setValue(val);
    //QSlider::set
    QSlider::setValue(val);
}

void GainSlider::changeEvent(QEvent *e)
{
    Q_UNUSED(e);
}

void GainSlider::sliderPressedH()
{
    sliderMovedH(sliderPosition());
}

void GainSlider::sliderMovedH(int val)
{
    double pos = (static_cast<double>(val) - static_cast<double>(minimum())) / static_cast<double>(maximum());

    int padding = 10;
    int wid = width() - padding;
    int xloc = pos * wid + padding/2;
    xloc -= popslide->width()/2;
    popslide->show();
    popslide->move(this->mapToGlobal(QPoint(xloc,-popslide->height())));
    popslide->setValue(val);
    popslide->setFocus();
}

void GainSlider::popValChanged(int v)
{
    setValue(v);
    emit sliderMoved(v);
}


