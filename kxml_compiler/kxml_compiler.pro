QT += xml network

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
RCC_DIR=build

DESTDIR = bin

SOURCES += \
    classdescription.cpp \
    creator.cpp \
    kxml_compiler.cpp \
    namer.cpp \
    parsercreatordom.cpp \
    parserrelaxng.cpp \
    parserxml.cpp \
    parserxsd.cpp \
    schema.cpp \
    writercreator.cpp

HEADERS += \
    writercreator.h \
    schema.h \
    parserxsd.h \
    parserxml.h \
    parserrelaxng.h \
    parsercreatordom.h \
    namer.h \
    creator.h \
    classdescription.h

INCLUDEPATH += ../
INCLUDEPATH += ../common
INCLUDEPATH += ../schema
INCLUDEPATH += ../libkode

LIBS += -lkode
LIBS += -L../libkode/bin
