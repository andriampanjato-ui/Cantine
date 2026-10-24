QT += core sql
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_metier

INCLUDEPATH += ..

SOURCES += \
    test_metier.cpp \
    ../database.cpp \
    ../clientmanager.cpp \
    ../platmanager.cpp \
    ../menumanager.cpp \
    ../commandemanager.cpp

HEADERS += \
    ../database.h \
    ../clientmanager.h \
    ../platmanager.h \
    ../menumanager.h \
    ../commandemanager.h
