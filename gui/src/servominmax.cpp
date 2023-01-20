
#include <QToolTip>
#include "servominmax.h"
#include "trackersettings.h"

ServoMinMax::ServoMinMax(QWidget *parent) : QWidget(parent)
{    
    min_travel = TrackerSettings::MIN_PWM; // In trackersettings.h
    max_travel = TrackerSettings::MAX_PWM;

    // Minimum value the maximum slider is allowed
    min_max = min_travel + TrackerSettings::MINMAX_RNG;

    // Maximum value the minimum slider is allowed
    max_min = max_travel - TrackerSettings::MINMAX_RNG;

    c_value = 1500;

    padding = 10;
    hpad = padding/2;
    sliderw  = 8;
    roundx = 1;
    roundy = 1;
    minSliderSelected = false;
    maxSliderSelected = false;
    centerSliderSelected = false;
    actValue = c_value; // Default Centered
    showPos = false;

    // Spin Boxes for direct entry
    cntspinbox = new PopupSlider(this);
    connect(cntspinbox,SIGNAL(valueChanged(double)),this,SLOT(cntSpinChanged(double)));
    minspinbox = new PopupSlider(this);
    minspinbox->setLimits(min_travel,min_max);
    connect(minspinbox,SIGNAL(valueChanged(double)),this,SLOT(minSpinChanged(double)));
    maxspinbox = new PopupSlider(this);
    maxspinbox->setLimits(max_min,max_travel);
    connect(maxspinbox,SIGNAL(valueChanged(double)),this,SLOT(maxSpinChanged(double)));
    cntspinbox->setLimits(min_travel + TrackerSettings::MIN_TO_CENTER,
                          max_travel - TrackerSettings::MIN_TO_CENTER);
    setDefaults();
    setContextMenuPolicy(Qt::CustomContextMenu);
    setMouseTracking(true);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),this, SLOT(showContextMenu(const QPoint &)));
}

ServoMinMax::~ServoMinMax()
{

}

void ServoMinMax::showContextMenu(const QPoint &pos)
{
       QMenu contextMenu(tr("Context menu"), this);

       QAction action1("Default Values", this);
       QAction action2("Re-Center", this);
       QAction action3("Maximum Values", this);

       connect(&action1, SIGNAL(triggered()), this, SLOT(setDefaults()));
       connect(&action2, SIGNAL(triggered()), this, SLOT(reCenter()));
       connect(&action3, SIGNAL(triggered()), this, SLOT(actSetMax()));
       contextMenu.addAction(&action1);
       contextMenu.addAction(&action3);
       contextMenu.addAction(&action2);
       contextMenu.exec(mapToGlobal(pos));
}

void ServoMinMax::reCenter()
{
    c_value = (max_val - min_val) /2 + min_val;
    emit(centerChanged(c_value));
    update();
}

void ServoMinMax::setDefaults()
{
    c_value = (TrackerSettings::MAX_PWM-TrackerSettings::MIN_PWM)/2 + TrackerSettings::MIN_PWM;
    min_val = TrackerSettings::DEF_MIN_PWM;
    max_val = TrackerSettings::DEF_MAX_PWM;

    update();
    emit minimumChanged(min_val);
    emit maximumChanged(max_val);
    emit centerChanged(c_value);
}

void ServoMinMax::actSetMax()
{
    c_value = (TrackerSettings::MAX_PWM-TrackerSettings::MIN_PWM)/2 + TrackerSettings::MIN_PWM;
    min_val = TrackerSettings::MIN_PWM;
    max_val = TrackerSettings::MAX_PWM;

    update();
    emit minimumChanged(min_val);
    emit maximumChanged(max_val);
    emit centerChanged(c_value);
}

void ServoMinMax::cntSpinChanged(double v)
{
    c_value = v;
    cntspinbox->move(this->mapToGlobal(QPoint(centerSlider.x()-((max_val - min_travel) / travel)-cntspinbox->width()/2+centerSlider.width()/2,-cntspinbox->height()+padding/2)));
    minMaxPositionCheck();
    update();
    emit centerChanged(c_value);
}

void ServoMinMax::minSpinChanged(double v)
{
    min_val = v;    
    minspinbox->move(this->mapToGlobal(QPoint(minSlider.x()-((max_val - min_travel) / travel)-minspinbox->width()/2+minSlider.width()/2,-minspinbox->height()+padding/2)));
    minMaxPositionCheck();
    update();
    emit minimumChanged(min_val);
}

void ServoMinMax::maxSpinChanged(double v)
{
    max_val = v;
    maxspinbox->move(this->mapToGlobal(QPoint(maxSlider.x()-((max_val - min_travel) / travel)-maxspinbox->width()/2+maxSlider.width()/2,-maxspinbox->height()+padding/2)));
    minMaxPositionCheck();
    update();
    emit maximumChanged(max_val);
}

void ServoMinMax::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QBrush brush;
    QPen pen;

    travel = max_travel - min_travel; // Overall range
    minSliderPos = (min_val - min_travel) / travel;
    maxSliderPos = (max_val - min_travel) / travel;
    centerSliderPos = (c_value - min_travel) / travel;
    minmaxp = (min_max - min_travel) / travel;
    maxminp = (max_min - min_travel) / travel;

    minmaxp = ((width()-padding) * minmaxp) + hpad;
    maxminp = ((width()-padding) * maxminp) + hpad;

    // Setup Slider Rectangles
    minSlider.setTop(hpad);
    minSlider.setBottom(height() - hpad);
    minSlider.setX((width()-padding) * minSliderPos + hpad);
    minSlider.setWidth(sliderw);
    minSlider.translate(-sliderw/2,0);

    maxSlider.setTop(hpad);
    maxSlider.setBottom(height() - hpad);
    maxSlider.setX((width()-padding) * maxSliderPos + hpad);
    maxSlider.setWidth(sliderw);
    maxSlider.translate(-sliderw/2,0);

    centerSlider.setTop(hpad);
    centerSlider.setBottom(height() - hpad);
    centerSlider.setX((width()-padding) * centerSliderPos + hpad);
    centerSlider.setWidth(sliderw);
    centerSlider.translate(-sliderw/2,0);

    double linetop = height() * 4/12;
    double linebot = height() * 8/12;

    // Max Travel Settings
    painter.drawLine(hpad,height()/2,width()-hpad,height()/2);
    // Left Line
    painter.drawLine(hpad,linetop,hpad,linebot);
    // Right Line
    painter.drawLine(width()-hpad,linetop,width()-hpad,linebot);


    // Draw MinMaxBar
    QRectF minmaxrect(minSlider.center().x(),linetop,maxSlider.center().x()-minSlider.center().x(),linebot-linetop);
    QLinearGradient minmaxbar(minmaxrect.x(),0,minmaxrect.right(),0);
    QColor minmaxcolor = qRgb(182, 255, 153);
    minmaxbar.setColorAt(0,minmaxcolor.darker(130));
    minmaxbar.setColorAt(0.5,minmaxcolor);
    minmaxbar.setColorAt(1,minmaxcolor.darker(130));
    painter.setBrush(minmaxbar);
    pen.setWidth(0);
    painter.setPen(pen);
    painter.drawRect(minmaxrect);

    // Draw Left Slider
    painter.setPen(Qt::NoPen);
    QLinearGradient lg(0,0,0,minSlider.height()-5);
    QColor color(Qt::blue);
    lg.setColorAt(0,color.lighter(130));
    lg.setColorAt(0.5,color);
    lg.setColorAt(1,color.lighter(130));
    painter.setBrush(lg);
    painter.drawRoundedRect(minSlider,roundx,roundy);

    // Draw Right Slider
    painter.drawRoundedRect(maxSlider,roundx,roundy);

    // Draw Center Slider
    color = Qt::red;
    lg.setColorAt(0,color.darker());
    lg.setColorAt(0.5,color);
    lg.setColorAt(1,color.darker());
    painter.setBrush(lg);
    painter.drawRoundedRect(centerSlider,roundx,roundy);

    // Draw the current output
    if(showPos) {
        painter.save();
        painter.setBrush(Qt::yellow);
        painter.setPen(QPen(Qt::black,1,Qt::SolidLine));
        double triheight = (linebot - linetop)*0.75;
        double ap = (actValue - min_travel) / travel;
        ap = ((width()-padding) * ap) + hpad;
        painter.translate(ap,height()/2-1);
        painter.rotate(45);
        painter.drawRect(-triheight/2,-triheight/2,triheight,triheight);
        painter.restore();
    }
}

void ServoMinMax::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if(min_val != min_start && minSliderSelected) {
        //qDebug() << "Min Val" << min_val;
        emit minimumChanged(min_val);
    }
    if(max_val != max_start && maxSliderSelected) {
        //qDebug() << "Max Val" << max_val;
        emit maximumChanged(max_val);
    }
    if(c_value != cnt_start && centerSliderSelected) {
        //qDebug() << "Cnt Val" << c_value;
        emit centerChanged(c_value);
    }

    minSliderSelected = false;
    maxSliderSelected = false;
    centerSliderSelected = false;
}

void ServoMinMax::mousePressEvent(QMouseEvent *event)
{
    minSliderSelected = false;
    maxSliderSelected = false;
    centerSliderSelected = false;

    cntspinbox->hide();
    minspinbox->hide();
    maxspinbox->hide();

    if(event->buttons() == Qt::LeftButton) {
        if(minSlider.contains(event->pos())) {
            mouseDownPoint = event->globalPosition();
            minSliderSelected = true;
            min_start = min_val;
            minspinbox->show();
            minspinbox->move(this->mapToGlobal(QPoint(minSlider.x()-minspinbox->width()/2+minSlider.width()/2,-minspinbox->height()+padding/2)));
            minspinbox->setValue(min_val);

        } else if(maxSlider.contains(event->pos())) {
            mouseDownPoint = event->globalPosition();
            maxSliderSelected = true;
            max_start = max_val;
            maxspinbox->show();
            maxspinbox->move(this->mapToGlobal(QPoint(maxSlider.x()-maxspinbox->width()/2+maxSlider.width()/2,-maxspinbox->height()+padding/2)));
            maxspinbox->setValue(max_val);

        } else if(centerSlider.contains(event->pos())) {
            mouseDownPoint = event->globalPosition();
            centerSliderSelected = true;
            cnt_start = c_value;
            cntspinbox->show();
            cntspinbox->move(this->mapToGlobal(QPoint(centerSlider.x()-cntspinbox->width()/2+centerSlider.width()/2,-cntspinbox->height()+padding/2)));
            cntspinbox->setValue(c_value);
        } else {
        }
    }
}

void ServoMinMax::mouseMoveEvent(QMouseEvent *event)
{
    // Movement from mouse down point in X axis
    int xtrans = mouseDownPoint.x() - event->globalPosition().x();
    double xtravelperpixel = travel/(width()-padding);
    double xtr = xtrans*xtravelperpixel;

    if(minSliderSelected) {
        min_val = min_start - xtr;
        minMaxPositionCheck();
        minspinbox->move(this->mapToGlobal(QPoint(minSlider.x()-((min_val - min_travel) / travel)-minspinbox->width()/2+minSlider.width()/2,-minspinbox->height()+padding/2)));
        minspinbox->setValue(min_val);
        update();
    } else if (maxSliderSelected) {
        max_val = max_start - xtr;
        minMaxPositionCheck();
        maxspinbox->move(this->mapToGlobal(QPoint(maxSlider.x()-((max_val - min_travel) / travel)-maxspinbox->width()/2+maxSlider.width()/2,-maxspinbox->height()+padding/2)));
        maxspinbox->setValue(max_val);
        update();
    } else if (centerSliderSelected) {
        c_value = cnt_start - xtr;
        minMaxPositionCheck();
        cntspinbox->move(this->mapToGlobal(QPoint(centerSlider.x()-((c_value - min_travel) / travel)-cntspinbox->width()/2+centerSlider.width()/2,-cntspinbox->height()+padding/2)));
        cntspinbox->setValue(c_value);
        update();
    }
}

void ServoMinMax::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton) {
        if(centerSlider.contains(event->pos())) {
        }
    }
}

void ServoMinMax::minMaxPositionCheck()
{
    if(min_val < min_travel)
        min_val = min_travel;
    if(min_val > min_max)
        min_val = min_max;
    if(max_val > max_travel)
        max_val = max_travel;
    if(max_val < max_min)
        max_val = max_min;
    if(c_value > max_val - TrackerSettings::MIN_TO_CENTER)
        c_value = max_val - TrackerSettings::MIN_TO_CENTER;
    if(c_value < min_val + TrackerSettings::MIN_TO_CENTER)
        c_value = min_val + TrackerSettings::MIN_TO_CENTER;
}

bool ServoMinMax::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QPoint glbpoint = helpEvent->globalPos();
            glbpoint.setY(glbpoint.y()-40);
            if(minSlider.contains(helpEvent->pos())) {
                QToolTip::showText(glbpoint, QString::number(min_val) + "uS");
            } else if(maxSlider.contains(helpEvent->pos())) {
                QToolTip::showText(glbpoint, QString::number(max_val) + "uS");
            } else if(centerSlider.contains(helpEvent->pos())) {
                QToolTip::showText(glbpoint, QString::number(c_value) + "uS");
            } else {
                QToolTip::hideText();
                event->ignore();
            }
            return true;
        }
        return QWidget::event(event);
}
