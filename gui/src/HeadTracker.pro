QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 file_copies

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    calibratebno.cpp \
    firmware.cpp \
    main.cpp \
    mainwindow.cpp \
    led.cpp \
    graph.cpp \
    nanoblewidget.cpp \
    servominmax.cpp \
    signalbars.cpp \
    trackersettings.cpp \
    ucrc16lib.cpp

HEADERS += \
    calibratebno.h \
    firmware.h \
    mainwindow.h \
    led.h \
    graph.h \
    nanoblewidget.h \
    servominmax.h \
    signalbars.h \
    trackersettings.h \
    ucrc16lib.h

FORMS += \
    calibratebno.ui \
    firmware.ui \
    mainwindow.ui \
    nanoblewidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

COPIES += stylesheets
stylesheets.files = $$files("css/*.*")
stylesheets.path = $$OUT_PWD

RC_ICONS = IconFile.ico

# Add Correct OpenSSL Libraries to the output dir
CONFIG(debug, debug|release) {
    TARGET_PATH = $$OUT_PWD/debug
}
CONFIG(release, debug|release) {
    TARGET_PATH = $$OUT_PWD/release
}

# Only add on Windows, will have to manually add on linux
win32 {
    contains(QT_ARCH, x86_64) {
    COPIES += openssl
    openssl.files = $$files("lib_x64/*.dll")
    openssl.path = $$TARGET_PATH
    } else {
    COPIES += openssl
    openssl.files = $$files("lib_x86/*.dll")
    openssl.path = $$TARGET_PATH
    }
}

DISTFILES +=


