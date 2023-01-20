#ifndef SERVOMINMAX_H
#define SERVOMINMAX_H

#include <QWidget>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QStylePainter>
#include <QStyleOptionSlider>
#include <QSpinBox>
#include <QTimer>
#include "popupslider.h"

class ServoMinMax : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int maximum READ maximumValue WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(int minimum READ minimumValue WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int center READ centerValue WRITE setCenter NOTIFY centerChanged)

public:
    explicit ServoMinMax(QWidget *parent = nullptr);
    ~ServoMinMax();
    int centerValue() {return c_value;}
    int minimumValue() {return min_val;}
    int maximumValue() {return max_val;}
    int minTravel() {return min_travel;}
    int maxTravel() {return max_travel;}
    void setMinMax(int max) {min_max=max;}
    void setMaxMin(int min) {max_min=min;}

private slots:
    void showContextMenu(const QPoint &p);
    void reCenter();
    void setDefaults();
    void actSetMax();
    void cntSpinChanged(double);
    void minSpinChanged(double);
    void maxSpinChanged(double);

public slots:
    void setCenter(int value) {c_value = value;update();} // !Add Limit Check!
    void setMinimum(int value) {min_val = value;update();}
    void setMaximum(int value) {max_val = value;update();}
    void setShowActualPosition(bool on) {showPos = on;update();}
    void setActualPosition(int value) {actValue = value;update();}

signals:
    void minimumChanged(int value);
    void maximumChanged(int value);
    void centerChanged(int value);

private:
    PopupSlider *cntspinbox;
    PopupSlider *minspinbox;
    PopupSlider *maxspinbox;

    bool showPos;
    int min_travel;
    int max_travel;
    int c_value;
    int min_max;
    int min_val;
    int max_val;
    int max_min;
    int actValue;

    double padding;
    double hpad;
    double sliderw;
    double roundx;
    double roundy;
    double travel;
    double minSliderPos;
    double maxSliderPos;
    double centerSliderPos;
    double minmaxp;
    double maxminp;
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    QRectF minSlider;
    QRectF maxSlider;
    QRectF centerSlider;
    QPointF mouseDownPoint;
    QPointF sliderStartPoint;
    bool minSliderSelected;
    bool maxSliderSelected;
    bool centerSliderSelected;
    int min_start;
    int max_start;
    int cnt_start;
    QTimer hidepopup;

    void minMaxPositionCheck();
};

#endif // SERVOMINMAX_H
