#-------------------------------------------------
#
# Project created by QtCreator 2021-01-17T20:03:07
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = MyProject
TEMPLATE = app
CONFIG +=c++14
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    qcustomplot.cpp \
    dataaquire.cpp \
    fft.cpp

HEADERS += \
        mainwindow.h \
    qcustomplot.h \
    dataaquire.h \
    stdafx.h \
    usb3202.h \
    fft.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    mysources.qrc

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lUSB3202
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lUSB3202
#else:unix: LIBS += -L$$PWD/./ -lUSB3202

#INCLUDEPATH += $$PWD/.
#DEPENDPATH += $$PWD/.


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lUSB3202
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lUSB3202
else:unix: LIBS += -L$$PWD/./ -lUSB3202

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.
