#ifndef VECTORVIEWER_H
#define VECTORVIEWER_H

#include <QWidget>
#include <QOpenGLFunctions>
#include <QtOpenGL>
#include "trackersettings.h"

namespace Ui {
class VectorViewer;
}

class VectorOpenGL : public QOpenGLWidget
{
  Q_OBJECT
public:
  explicit VectorOpenGL(TrackerSettings *trk, QWidget *parent = nullptr);
public slots:
  void dataUpdate();
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private:
  TrackerSettings *trkset;
};


class VectorViewer : public QWidget
{
  Q_OBJECT

public:
  explicit VectorViewer(TrackerSettings *trk, QWidget *parent = nullptr);
  ~VectorViewer();

private:
  Ui::VectorViewer *ui;
  VectorOpenGL *vectGLWidget;
  TrackerSettings *trkset;
  void showEvent(QShowEvent *event);
};

#endif // VECTORVIEWER_H
