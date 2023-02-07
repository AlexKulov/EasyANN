QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: CONFIG += c++11 console
unix: CONFIG += c++11

TRANSLATIONS += languages/FannInterface_en_US.ts
TRANSLATIONS += languages/FannInterface_ru.ts
CODECFORSRC     = UTF-8

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \
    fann/fann.c \
    fann/fann_cascade.c \
    fann/fann_error.c \
    fann/fann_io.c \
    fann/fann_train.c \
    fann/fann_train_data.c

HEADERS += \
    mainwindow.h \
    qcustomplot.h \

FORMS += \
    mainwindow.ui

RESOURCES += \
  languages/Languages.qrc

