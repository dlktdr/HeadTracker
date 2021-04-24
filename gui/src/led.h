#ifndef LED_H
#define LED_H

#include <QWidget>
#include <QPainter>
#include <QTimer>

class Led : public QWidget
{
    Q_OBJECT
private:
    QColor color,colon,coloff,colblink;
    QTimer blinktmr;
    bool blink;
    bool blinkstate;
    bool state;

private slots:
    void blinkTimeout();

public:
    explicit Led(QWidget *parent = nullptr);
    void setState(bool);
    void setBlink(bool b) {blink = b;}
    void setOnColor(QColor a) {colon = a;}
    void setOffColor(QColor a) {coloff = a;}
    void setBlinkolor(QColor a) {colblink = a;}
signals:

protected:
    void paintEvent(QPaintEvent *event) override;


};

#endif // LED_H
