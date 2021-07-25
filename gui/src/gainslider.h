#ifndef GAINSLIDER_H
#define GAINSLIDER_H

#include <QSlider>
#include <QObject>
#include <popupslider.h>

class GainSlider : public QSlider
{
    Q_OBJECT
public:
    GainSlider(QWidget *parent = nullptr);
    ~GainSlider();
    void setSliderPosition(int);
    void setMaximum(int);
    void setMinimum(int);
public slots:
    void setValue(int);

protected:
    void changeEvent(QEvent *e);

private:
    PopupSlider *popslide;
private slots:
    void sliderPressedH();
    void sliderMovedH(int);
    void popValChanged(double);
};

#endif // GAINSLIDER_H
