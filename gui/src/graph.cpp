#include "graph.h"
const int pointcount = 100;

Graph::Graph(QWidget *parent) : QWidget(parent)
{

}

void Graph::addDataPoints(int x, int y, int z)
{    
    pointstilt.append(x);
    pointsroll.append(y);
    pointspan.append(z);

    int points = pointstilt.length();

    if(points > pointcount*1.5) {
        pointstilt = pointstilt.mid(points-pointcount);
    }
    if(points > pointcount*1.5) {
        pointsroll = pointsroll.mid(points-pointcount);
    }
    if(points > pointcount*1.5) {
        pointspan = pointspan.mid(points-pointcount);
    }

    update();
}

void Graph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPen pen;

    painter.setRenderHint(QPainter::Antialiasing,true);

    // Background
    QLinearGradient lg(0,0,0,height());
    QColor bg(qRgb(153, 201, 255));
    lg.setColorAt(0,bg.lighter(120));
    lg.setColorAt(0.5,bg);
    lg.setColorAt(1,bg.lighter(120));
    painter.setBrush(QBrush(lg));
    painter.drawRect(rect().adjusted(0,0,-1,-1));

    // Zero Line

    painter.setRenderHint(QPainter::Antialiasing,false);
    pen.setColor(Qt::black);
    pen.setWidth(2);
    painter.drawLine(0,height()/2,width(),height()/2);
    painter.setRenderHint(QPainter::Antialiasing,true);

    if(pointstilt.length() < 2)
        return;

    // Graph Lines
    QPainterPath tilpath;
    QPainterPath rllpath;
    QPainterPath panpath;
    for(int i=1; i < pointstilt.length(); i++) {
        int tval = pointstilt.at(pointstilt.length()-i);
        int pval = pointspan.at(pointspan.length()-i);
        int rval = pointsroll.at(pointsroll.length()-i);
        qreal ty = height()/2 - (height()*(tval)/360);
        qreal py = height()/2 - (height()*(pval)/360);
        qreal ry = height()/2 - (height()*(rval)/360);
        if(i==1) {
            tilpath.moveTo(width(),ty);
            rllpath.moveTo(width(),ry);
            panpath.moveTo(width(),py);
        }
         else {
            tilpath.lineTo(width() - (width()*i/pointcount),ty);
            rllpath.lineTo(width() - (width()*i/pointcount),ry);
            panpath.lineTo(width() - (width()*i/pointcount),py);
        }
    }


    painter.setBrush(Qt::NoBrush);

    // Tilt Red on Black
    pen.setColor(Qt::black);
    pen.setWidth(4);
    painter.setPen(pen);
    painter.drawPath(tilpath);
    pen.setColor(Qt::red);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawPath(tilpath);


    // Roll Green on Black
    pen.setColor(Qt::black);
    pen.setWidth(4);
    painter.setPen(pen);
    painter.drawPath(rllpath);
    pen.setWidth(2);
    pen.setColor(Qt::green);
    painter.setPen(pen);
    painter.drawPath(rllpath);


    // Pan Yellow on Black
    pen.setColor(Qt::black);
    pen.setWidth(4);
    painter.setPen(pen);
    painter.drawPath(panpath);
    pen.setWidth(2);
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    painter.drawPath(panpath);

    // Legend
    pen.setColor(Qt::black);
    painter.setPen(pen);
    QFont legfont = QFont("Times", 10, QFont::Bold);
    const int offset=6;
    int fonth = QFontMetrics(legfont).height();    
    painter.setFont(legfont);
    painter.drawText(10,offset+fonth/2,"+180"); // Top
    painter.drawText(10,(height()/2),"0"); // Center
    painter.drawText(10,height()-offset,"-180");
    painter.setBrush(Qt::black);
    painter.setPen(Qt::white);
    painter.drawRect(width()-47,2,38,50);
    pen.setColor(Qt::red);
    painter.setPen(pen);
    painter.drawText( width() - 40,17, "Tilt");
    pen.setColor(Qt::green);
    painter.setPen(pen);
    painter.drawText( width() - 40,32, "Roll");
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    painter.drawText( width() - 40,47, "Pan");

}

