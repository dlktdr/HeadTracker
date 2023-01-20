QT       += core gui svg serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl openglwidgets

CONFIG += c++11 file_copies

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_NO_DEPRECATED_WARNING
DEFINES += GIT_CURRENT_SHA=$$system($$quote(git rev-parse --short HEAD))
DEFINES += GIT_VERSION_TAG=$$system($$quote(git describe --tags --abbrev=0))

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    boardbno055.cpp \
    boardnano33ble.cpp \
    boardtype.cpp \
    calibrate/fusion.cpp \
    calibrate/imuread.cpp \
    calibrate/magcal.cpp \
    calibrate/mahony.cpp \
    calibrate/matrix.cpp \
    calibrate/quality.cpp \
    calibrate/rawdata.cpp \
    calibrateble.cpp \
    calibratebno.cpp \
    channelviewer.cpp \
    diagnosticdisplay.cpp \
    firmwarewizard.cpp \
    gainslider.cpp \
    imageviewer/imageviewer.cpp \
    magcalwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    led.cpp \
    graph.cpp \
    popupslider.cpp \
    servominmax.cpp \
    signalbars.cpp \
    trackersettings.cpp \
    ucrc16lib.cpp


HEADERS += \
    boardbno055.h \
    boardnano33ble.h \
    boardtype.h \
    calibrate/imuread.h \
    calibrateble.h \
    calibratebno.h \
    channelviewer.h \
    diagnosticdisplay.h \
    firmwarewizard.h \
    gainslider.h \
    imageviewer/imageviewer.h \
    magcalwidget.h \
    mainwindow.h \
    led.h \
    graph.h \
    popupslider.h \
    servominmax.h \
    signalbars.h \
    trackersettings.h \
    ucrc16lib.h \
    basetrackersettings.h

FORMS += \
    calibrateble.ui \
    calibratebno.ui \
    channelviewer.ui \
    diagnosticdisplay.ui \
    firmwarewizard.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

COPIES += stylesheets
stylesheets.files = $$files("css/*.*")
stylesheets.path = $$OUT_PWD

RC_ICONS = images/IconFile.ico

# Add Correct OpenSSL Libraries to the output dir
CONFIG(debug, debug|release) {
    TARGET_PATH = $$OUT_PWD/debug
}
CONFIG(release, debug|release) {
    TARGET_PATH = $$OUT_PWD/release
}

# Windows specific
win32 {
    DEFINES += "WINDOWS=yes"
    LIBS += -lOpengl32
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

#Linux Specific
unix:!macx {
    DEFINES += "LINUX=yes"
}

#Mac Specific
macx: {
    DEFINES += "MACOS=yes"
    ICON = images/iconbuilder.icns
}

DISTFILES += \
    Revisions.txt


