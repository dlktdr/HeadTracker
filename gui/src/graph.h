#ifndef GRAPH_H
#define GRAPH_H

#include <QWidget>
#include <QPainter>
#include <QMap>
#include <QPainterPath>
#include <QLinearGradient>

class Graph : public QWidget
{
    Q_OBJECT
private:
    QList<int> pointsroll;
    QList<int> pointstilt;
    QList<int> pointspan;
public:
    explicit Graph(QWidget *parent = nullptr);

    void addDataPoints(int x,int y,int z);
signals:


protected:
    void paintEvent(QPaintEvent *event) override;

};

#endif // GRAPH_H
