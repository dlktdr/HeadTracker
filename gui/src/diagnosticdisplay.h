#ifndef DIAGNOSTICDISPLAY_H
#define DIAGNOSTICDISPLAY_H

#include <QWidget>
#include <QStandardItemModel>
#include "trackersettings.h"

namespace Ui {
class DiagnosticDisplay;
}

class DataModel;

class DiagnosticDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticDisplay(TrackerSettings *ts, QWidget *parent = nullptr);
    ~DiagnosticDisplay();

private:
    Ui::DiagnosticDisplay *ui;
     TrackerSettings *trkset;
     QVariantMap livedata;
     DataModel *model;
public slots:
     void updated();
};

class DataItem
{
public:
    QString name;
    QVariant value;
    bool checked;
    QModelIndex index0;
    QModelIndex index1;
};

class DataModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  DataModel(TrackerSettings *ts, QObject *parent = nullptr);
  ~DataModel();
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override {Q_UNUSED(parent); return datalist.count()-1;}
  int columnCount(const QModelIndex &parent = QModelIndex()) const override {Q_UNUSED(parent); return 3;}
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
private:
  void checkArray(QString array, bool checked);
  TrackerSettings *trkset;
  QList<DataItem *> datalist;
private slots:
  void dataupdate();

};

#endif // DIAGNOSTICDISPLAY_H
