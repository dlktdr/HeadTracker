#include "vectorviewer.h"

VectorViewer::VectorViewer(TrackerSettings *trk, const QUrl &source, QWindow *parent) :
  QQuickView(source, parent),
  trkset(trk)
{

}

void VectorViewer::showEvent(QShowEvent *event)
{
  // Save currently sending Items - TODO
  //   on close of window restore them
  trkset->clearDataItems();

  QMap<QString, bool> dataitems;
  dataitems.insert("off_accx",true);
  dataitems.insert("off_accy",true);
  dataitems.insert("off_accz",true);

  dataitems.insert("off_magx",true);
  dataitems.insert("off_magy",true);
  dataitems.insert("off_magz",true);

  trkset->setDataItemSend(dataitems);
  event->accept();
}
