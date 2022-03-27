#ifndef MAGCALWIDGET_H
#define MAGCALWIDGET_H

#include <QOpenGLWidget>
#include "calibrate/imuread.h"
#include "trackersettings.h"


class MagCalWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MagCalWidget(QWidget *parent = nullptr);
    void setTracker(TrackerSettings *trkset);
    void resetDataPoints();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initalizeGL();
    TrackerSettings *trkset;

    GLuint spherelist;
    GLuint spherelowreslist;
    void drawSphere(double r, int lats, int longs);

    int invert_q0=0;
    int invert_q1=0;
    int invert_q2=0;
    int invert_q3=1;
    int invert_x=0;
    int invert_y=0;
    int invert_z=0;
    int points=0;

private slots:
    void rawMagChanged(float x, float y, float z);
signals:
    void dataUpdate(float variance,
                    float gaps,
                    float wobble,
                    float fiterror,
                    float hoop[3],
                    float soo[3][3]);

};

#endif // MAGCALWIDGET_H
