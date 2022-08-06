#ifndef VECTORVIEWER_H
#define VECTORVIEWER_H

#include <QQuickView>
#include "trackersettings.h"

#include <QtQml/qqml.h>

class VectorViewer : public QQuickView
{
  Q_OBJECT

public:
  explicit VectorViewer(TrackerSettings *trk,
                        const QUrl &source,
                        QWindow *parent = nullptr);

private:
  TrackerSettings *trkset;
  void showEvent(QShowEvent *event);

};

#endif // VECTORVIEWER_H
