#include "gainslider.h"

GainSlider::GainSlider(QWidget *parent) : QSlider(parent)
{
    popslide = new PopupSlider(this);
    popslide->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    popslide->setSuffix(" us/deg");
    popslide->setPrecision(1);
    popslide->setStep(0.11);

    connect(this,&QSlider::sliderPressed,this,&GainSlider::sliderPressedH);
    connect(this,&QSlider::sliderMoved,this,&GainSlider::sliderMovedH);
    connect(popslide,&PopupSlider::valueChanged,this,&GainSlider::popValChanged);
    setMouseTracking(true);
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
    popslide->setLimits(minimum()/10,maximum()/10);
}

void GainSlider::setMinimum(int val)
{
    QSlider::setMinimum(val);
    popslide->setLimits(minimum()/10,maximum()/10);
}

void GainSlider::setValue(int val)
{
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
    // Move popupslider to match the bar
    double pos = (static_cast<double>(val) - static_cast<double>(minimum())) / static_cast<double>(maximum());
    int padding = 10;
    int wid = width() - padding;
    int xloc = pos * wid + padding/2;
    xloc -= popslide->width()/2;
    popslide->show();
    popslide->move(this->mapToGlobal(QPoint(xloc,-popslide->height())));

    // Set the value
    popslide->setValue(static_cast<double>(val)/10);
}

void GainSlider::popValChanged(double v)
{
    setValue(v*10);
    emit sliderMoved(value());
}


