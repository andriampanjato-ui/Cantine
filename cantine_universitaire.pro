QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = CantineUniversitaire
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    database.cpp \
    dashboard.cpp \
    clientmanager.cpp \
    platmanager.cpp \
    menumanager.cpp \
    commandemanager.cpp \
    clientspage.cpp \
    platspage.cpp \
    menupage.cpp \
    neworderpage.cpp \
    historypage.cpp

HEADERS += \
    mainwindow.h \
    database.h \
    dashboard.h \
    clientmanager.h \
    platmanager.h \
    menumanager.h \
    commandemanager.h \
    clientspage.h \
    platspage.h \
    menupage.h \
    neworderpage.h \
    historypage.h \
    utils.h

RESOURCES += \
    resources.qrc

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
