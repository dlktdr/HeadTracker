#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication a(argc, argv);

    // Check in two locations for stylesheet. Current Working Dir & Binary Directory
    QString cssFileName = "stylesheet.css";
    bool fileOpen=false;

    QFile fileStyleSheet(cssFileName);
    if(fileStyleSheet.open(QIODevice::ReadOnly| QIODevice::Text)) {
        fileOpen = true;
    } else {
      cssFileName = QCoreApplication::applicationDirPath() + "/stylesheet.css";
      fileStyleSheet.setFileName(cssFileName);
      if(fileStyleSheet.open(QIODevice::ReadOnly| QIODevice::Text)) {
        fileOpen = true;
      }
    }

    if(fileOpen) {
      a.setStyleSheet(fileStyleSheet.readAll());
      qDebug() << "Using stylesheet " << cssFileName;
      fileStyleSheet.close();
    } else {
      qDebug() << cssFileName << " not found - " + fileStyleSheet.errorString();
    }

    MainWindow w;

    w.show();
    return a.exec();
}
