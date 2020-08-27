#-------------------------------------------------
#
# Project created by QtCreator 2020-08-05T19:30:00
#
#-------------------------------------------------
QT -= gui
QT += core widgets x11extras

TEMPLATE = lib
TARGET = a11y-settings

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        glib-2.0    \
        gio-2.0     \
        gsettings-qt

INCLUDEPATH += \
    -I $$PWD/../../common       \
    -I ukui-settings-daemon/
LIBS +=

SOURCES += \
    $$PWD/a11ysettingsmanager.cpp \
    $$PWD/a11ysettingsplugin.cpp

HEADERS += \
    $$PWD/a11ysettingsmanager.h \
    $$PWD/a11ysettingsplugin.h

a11_settings_lib.path = $${PLUGIN_INSTALL_DIRS}
a11_settings_lib.files = $$OUT_PWD/liba11y-settings.so

INSTALLS += a11_settings_lib
