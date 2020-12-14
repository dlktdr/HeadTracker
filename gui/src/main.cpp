#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile stylesheet("stylesheet.css");
    if(stylesheet.open(QIODevice::ReadOnly| QIODevice::Text)) {
        qDebug() << "Using stylesheet.css";
        a.setStyleSheet(stylesheet.readAll());
    } else {
        qDebug() << "stylesheet.css not found - " + stylesheet.errorString();
    }

    MainWindow w;
    //QObject::connect(&w, &QMainWindow::, &a, &QApplication::quit);
    w.show();
    return a.exec();
}
