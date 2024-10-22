#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HeadTracker_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QString cssFileName = ":/css/stylesheet.css";

    // Force Current Working Dir to Same location as the Executable
    //  So we can find stylesheet and images easily on all platforms
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QFile fileStyleSheet(cssFileName);
    if(fileStyleSheet.open(QIODevice::ReadOnly| QIODevice::Text)) {
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
