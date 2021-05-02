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
    ui->tblLiveData->verticalHeader()->setVisible(false);
    ui->tblLiveData->horizontalHeader()->setVisible(false);
    ui->tblLiveData->horizontalHeader()->setStretchLastSection(true);
    ui->tblLiveData->setColumnWidth(0,100);
}

DiagnosticDisplay::~DiagnosticDisplay()
{
    delete ui;
}

void DiagnosticDisplay::updated()
{
    QVariantMap livedata=trkset->liveDataMap();
    QVariantMap settings=trkset->allData();

    ui->tblParams->clear();
    ui->tblParams->setColumnCount(2);
    ui->tblParams->setRowCount(settings.count());
    ui->tblLiveData->clear();
    ui->tblLiveData->setColumnCount(2);
    ui->tblLiveData->setRowCount(livedata.count());

    int row=0;
    QMapIterator<QString, QVariant> i(livedata);
    while (i.hasNext()) {
        i.next();
        QTableWidgetItem *key = new QTableWidgetItem(i.key());
        QTableWidgetItem *value = new QTableWidgetItem(i.value().toString());
        if(i.key() == "Cmd") {
            key->setBackground(QBrush(Qt::lightGray));
            value->setBackground(QBrush(Qt::lightGray));
        }
        ui->tblLiveData->setItem(row,0,key);
        ui->tblLiveData->setItem(row++,1,value);
    }
    row=0;
    QMapIterator<QString, QVariant> x(settings);
    while (x.hasNext()) {
        x.next();
        QTableWidgetItem *key = new QTableWidgetItem(x.key());
        QTableWidgetItem *value = new QTableWidgetItem(x.value().toString());
        if(x.key() == "Cmd") {
            key->setBackground(QBrush(Qt::lightGray));
            value->setBackground(QBrush(Qt::lightGray));
        }

        ui->tblParams->setItem(row,0,key);
        ui->tblParams->setItem(row++,1,value);
    }



}
