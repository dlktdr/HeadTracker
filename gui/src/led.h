#ifndef LED_H
#define LED_H

#include <QWidget>
#include <QPainter>

class Led : public QWidget
{
    Q_OBJECT
private:
    QColor color,colon,coloff;

public:
    explicit Led(QWidget *parent = nullptr);
    void setState(bool);
    void setOnColor(QColor a) {colon = a;}
    void setOffColor(QColor a) {coloff = a;}
signals:

protected:
    void paintEvent(QPaintEvent *event) override;


};

#endif // LED_H
