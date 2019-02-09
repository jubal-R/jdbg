TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -L.local/lib -lreadline

SOURCES += main.cpp \
    debugger.cpp \
    breakpoint.cpp \
    core.cpp

HEADERS += \
    debugger.h \
    breakpoint.h \
    core.h
