#-------------------------------------------------
#
# Project created by QtCreator 2016-12-01T23:50:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LoopSubdiv
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    mainview.cpp \
    meshrenderer.cpp \
    objfile.cpp \
    mesh.cpp \
    meshtools.cpp \
    settings.cpp

HEADERS  += mainwindow.h \
    mainview.h \
    face.h \
    mesh.h \
    meshrenderer.h \
    renderer.h \
    settings.h \
    vertex.h \
    halfedge.h \
    meshtools.h \
    objfile.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
