#-------------------------------------------------
#
# Project created by QtCreator 2016-07-17T23:00:32
#
#-------------------------------------------------

QT       += core gui opengl widgets

TARGET = MiniPhysics
TEMPLATE = app

CONFIG += c++11

DESTDIR = $$PWD/bin

CONFIG(release, debug|release):message(Release build!) #will print
CONFIG(debug, debug|release):message(Debug build!) #no print

QMAKE_CXXFLAGS += -ffloat-store

DEFINES += PGE_FILES_QT

include("$$PWD/PGE_File_Formats/File_FormatsQT.pri");

SOURCES += main.cpp\
        mainwindow.cpp \
        miniphysics.cpp \
    common_features/point.cpp \
    common_features/pointf.cpp \
    common_features/rect.cpp \
    common_features/rectf.cpp \
    common_features/size.cpp \
    common_features/sizef.cpp

HEADERS  += mainwindow.h \
    miniphysics.h \
    common_features/point.h \
    common_features/pointf.h \
    common_features/rect.h \
    common_features/rectf.h \
    common_features/size.h \
    common_features/sizef.h

FORMS    += mainwindow.ui

RESOURCES += \
    res.qrc

