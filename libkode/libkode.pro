#-------------------------------------------------
#
# Project created by QtCreator 2017-07-11T23:05:09
#
#-------------------------------------------------

QT       += xml network

QT       -= gui

TARGET = kode
TEMPLATE = lib

DEFINES += LIBKODE_LIBRARY

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
RCC_DIR=build

DESTDIR = bin

CONFIG += DEBUG

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(libkode.pri)
include(../common/common.pri)
include(../schema/schema.pri)

unix {
    target.path = /usr/lib
    INSTALLS += target
}


INCLUDEPATH += "../common"
