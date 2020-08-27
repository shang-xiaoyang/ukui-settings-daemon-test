#-------------------------------------------------
#
# Project created by QtCreator 2020-04-10T09:30:00
#
#-------------------------------------------------
TEMPLATE = lib
TARGET = clipboard

QT += gui
CONFIG += no_keywords c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \

PKGCONFIG += \
        gdk-3.0

SOURCES += \
    $$PWD/list.c \
    $$PWD/xutils.c \
    $$PWD/clipboard-plugin.cpp \
    $$PWD/clipboard-manager.cpp \

HEADERS += \
    $$PWD/list.h \
    $$PWD/xutils.h \
    $$PWD/clipboard-plugin.h \
    $$PWD/clipboard-manager.h

clipboard_lib.path = $${PLUGIN_INSTALL_DIRS}
clipboard_lib.files = $$OUT_PWD/libclipboard.so

INSTALLS += clipboard_lib
