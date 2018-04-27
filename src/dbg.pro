TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -L.local/lib -lreadline

SOURCES += main.cpp \
    debugger.cpp

HEADERS += \
    debugger.h
