#include "vectorviewer.h"

VectorViewer::VectorViewer(TrackerSettings *trk, const QUrl &source, QWindow *parent) :
  QQuickView(source, parent),
  trkset(trk)
{

}

void VectorViewer::showEvent(QShowEvent *event)
{
  // Store previous items
  lastDataItems = trkset->getDataItems();

  // Clear them all
  trkset->clearDataItems();

  QMap<QString, bool> dataitems;
  dataitems.insert("off_accx",true);
  dataitems.insert("off_accy",true);
  dataitems.insert("off_accz",true);

  dataitems.insert("off_magx",true);
  dataitems.insert("off_magy",true);
  dataitems.insert("off_magz",true);

  dataitems.insert("tiltoff",true);
  dataitems.insert("rolloff",true);
  dataitems.insert("panoff",true);

  trkset->setDataItemSend(dataitems);
  event->accept();
}

void VectorViewer::hideEvent(QHideEvent *evt)
{
  restoreDataItems();
  evt->accept();
}

void VectorViewer::restoreDataItems()
{
  trkset->clearDataItems();
  trkset->setDataItemSend(lastDataItems);
}
