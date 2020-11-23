#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QScrollBar>
#include <QSerialPortInfo>
#include "trackersettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialcon;
    TrackerSettings trkset;
    QString serialData;
    QTimer rxledtimer;
    QTimer txledtimer;

    int xtime;
    void parseSerialData();
    bool graphing;

    void sendSerialData(QString data);

private slots:
    void findSerialPorts();
    void serialConnect();
    void serialDisconnect();
    void updateFromUI();
    void updateToUI();
    void serialReadReady();
    void manualSend();
    void startGraph();
    void stopGraph();
    void uiSettingChanged();
    void storeSettings();

    void rxledtimeout();
    void txledtimeout();
};
#endif // MAINWINDOW_H
