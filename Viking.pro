#-------------------------------------------------
#
# Project created by QtCreator 2013-02-22T19:07:57
#
#-------------------------------------------------

QT += core gui

greaterThan( QT_MAJOR_VERSION, 4) : QT += widgets

TARGET = Viking
TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp \
    plotter.cpp \
    signals.cpp

HEADERS += mainwindow.h \
    plotter.h \
    signals.h \
    signal_parameters.h

FORMS += mainwindow.ui

RESOURCES += Viking.qrc

TRANSLATIONS = Viking_ru.ts
