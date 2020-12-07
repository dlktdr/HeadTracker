#ifndef SIGNALBARS_H
#define SIGNALBARS_H

#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QStyleOptionFrame>

class SignalBars : public QWidget
{
    Q_OBJECT
public:
    explicit SignalBars(QWidget *parent = nullptr);

public slots:
    void setSignal(int v) {signal = v;update();}

signals:

private:
    void paintEvent(QPaintEvent *event);
    int signal;
};

#endif // SIGNALBARS_H
