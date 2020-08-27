#-------------------------------------------------
#
# Project created by QtCreator 2020-05-30T09:30:00
#
#-------------------------------------------------
QT       += dbus
QT       -= gui
TEMPLATE = lib
TARGET = xsettings

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG +=\
    glib-2.0 \
    gio-2.0 \
    gdk-3.0 \
    atk

INCLUDEPATH += \
    -I $$PWD/../../common

SOURCES += \
    ukui-xsettings-plugin.cpp \
    xsettings-manager.cpp \
    ukui-xsettings-manager.cpp \
    ukui-xft-settings.cpp \
    xsettings-common.c \
    fontconfig-monitor.c

HEADERS += \
    ukui-xsettings-plugin.h \
    xsettings-manager.h \
    ukui-xsettings-manager.h \
    ukui-xft-settings.h \
    xsettings-const.h \
    xsettings-common.h  \
    fontconfig-monitor.h


xsettings_lib.path = $${PLUGIN_INSTALL_DIRS}
xsettings_lib.files += $$OUT_PWD/libxsettings.so

INSTALLS += xsettings_lib
