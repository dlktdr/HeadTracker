#include <QDebug>
#include "diagnosticdisplay.h"
#include "ui_diagnosticdisplay.h"

DiagnosticDisplay::DiagnosticDisplay(TrackerSettings *ts, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DiagnosticDisplay),
    trkset(ts)
{
    ui->setupUi(this);
    connect(trkset,&TrackerSettings::liveDataChanged,this,&DiagnosticDisplay::updated);
    updated();
    ui->tblParams->verticalHeader()->setVisible(false);
    ui->tblParams->horizontalHeader()->setVisible(false);
    ui->tblParams->horizontalHeader()->setStretchLastSection(true);
    ui->tblParams->setColumnWidth(0,100);
    //buildSettingsModel();

    model = new DataModel(trkset);
    ui->tblLiveData->setModel(model);
    //ui->tblLiveData->setStyleSheet("QTreeView::item {padding-left: 0px; border: 0px}");
    ui->tblLiveData->resizeColumnToContents(0);
    ui->tblLiveData->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);

    ui->tabWidget->setCurrentIndex(0);
}

DiagnosticDisplay::~DiagnosticDisplay()
{
  delete ui;
}

void DiagnosticDisplay::updated()
{
    QVariantMap settings=trkset->allData();

    ui->tblParams->clear();
    ui->tblParams->setColumnCount(3);
    ui->tblParams->setRowCount(settings.count());

    int row=0;
    QMapIterator<QString, QVariant> x(settings);
    while (x.hasNext()) {
        x.next();
        QTableWidgetItem *key = new QTableWidgetItem(x.key());
        QTableWidgetItem *value = new QTableWidgetItem(x.value().toString());
        QTableWidgetItem *description = new QTableWidgetItem(trkset->descriptions[x.key()]);
        if(x.key() == "Cmd") {
            key->setBackground(QBrush(Qt::lightGray));
            value->setBackground(QBrush(Qt::lightGray));
            description->setBackground(QBrush(Qt::lightGray));
            description->setText("Description");
        }

        ui->tblParams->setItem(row,0,key);
        ui->tblParams->setItem(row,1,value);
        ui->tblParams->setItem(row++,2,description);
    }
}

DataModel::DataModel(TrackerSettings *ts, QObject *parent)
  : QAbstractItemModel(parent)
{
  trkset = ts;
  int i=0;

  // Build all the data
  foreach(QString di, trkset->allDataItems()) {
    DataItem *dataitem = new DataItem;
    dataitem->checked = false;
    dataitem->name = di;
    dataitem->index0 = createIndex(i,0,dataitem);
    dataitem->index1 = createIndex(i++,1,dataitem);
    datalist.append(dataitem);
  }
  connect(trkset,&TrackerSettings::liveDataChanged, this, &DataModel::dataupdate);
}

DataModel::~DataModel()
{
  foreach(DataItem *itm, datalist) {
    delete itm;
  }
}

Qt::ItemFlags DataModel::flags(const QModelIndex &index) const
{
  if(index.column() == 0)
    return Qt::ItemIsUserCheckable |
           Qt::ItemIsEnabled;
  return Qt::ItemIsEnabled;
}

bool DataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(!index.isValid())
    return false;
  if(index.column() == 0 && role == Qt::CheckStateRole && index.row() < datalist.count()) {
    qDebug() << "User checked row" << index.row();
    DataItem *itm = datalist.at(index.row());
    bool checked = value==Qt::Checked?true:false;

    // Is it an array value
    QString arrname = itm->name.mid(0,itm->name.indexOf('['));
    if(arrname.size()) {
      trkset->setDataItemSend(arrname, checked);
      checkArray(arrname, checked); // Select them all
    // Single Values
    } else {
      itm->checked = checked;
      trkset->setDataItemSend(datalist.at(index.row())->name,itm->checked);
    }
    return true;
  }
  return false;
}

QVariant DataModel::data(const QModelIndex &index, int role) const
{

  if(index.row() >= datalist.count() - 1 || !index.isValid() )
    return QVariant();

  if(role == Qt::DisplayRole) {
    if(index.column() == 0) {
        return datalist.at(index.row())->name;
    }
    if(index.column() == 1) {
        return datalist.at(index.row())->value;
    }
    if(index.column() == 2) { // Description
        QString name = datalist.at(index.row())->name;
        int pos = name.indexOf(QChar('['));
        if(pos >= 0) {
            name = name.left(pos);
        }
        return trkset->descriptions[name];
    }
  } else if (role == Qt::CheckStateRole) {
      if(index.column() == 0)
        return datalist.at(index.row())->checked?Qt::Checked:Qt::Unchecked;
    }
  return QVariant();
}

QModelIndex DataModel::index(int row, int column, const QModelIndex &parent) const
{
  Q_UNUSED(parent)
  if(!(row < datalist.count()))
    return QModelIndex();

  if(column >=0 && column < 4)
    return createIndex(row,column);

  return QModelIndex();
}

QModelIndex DataModel::parent(const QModelIndex &index) const
{
  Q_UNUSED(index);
  return QModelIndex();
}

QVariant DataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role != Qt::DisplayRole ||
     orientation != Qt::Horizontal)
    return QVariant();
  if(section == 0)
    return tr("Item");
  if(section == 1)
    return tr("Value");
  if(section == 2)
    return tr("Description");
  return QVariant();
}

void DataModel::checkArray(QString array, bool checked)
{
  foreach(DataItem *itm, datalist) {
    QString arrname = itm->name.mid(0,itm->name.indexOf('['));
    if(arrname == array) {
      itm->checked = checked;
      Q_EMIT(dataChanged(itm->index0,itm->index0));
    }
  }
}

void DataModel::dataupdate()
{
  // Update local map of data
  QVariantMap liveDataMap = trkset->liveDataMap();
  QMapIterator<QString, QVariant> a(liveDataMap);
  while (a.hasNext()) {
    a.next();
    for(int i=0; i < datalist.count(); ++i) {
      if(datalist.at(i)->name == a.key()) {
        datalist[i]->value = a.value();
        Q_EMIT(dataChanged(datalist[i]->index1,datalist[i]->index1));
      }
    }
  }

  // Check all the currently sending items
  QMapIterator<QString, bool> b(trkset->getDataItems());
  while (b.hasNext()) {
    b.next();
    // Loop through all StandardItems to find a match
    for(int i=0; i < datalist.count(); i++) {
      DataItem *itm = datalist.at(i);
      if(b.key() == itm->name) {
        itm->checked = b.value();
          Q_EMIT(dataChanged(itm->index0,itm->index0));
        break;
      }
    }
  }
}

