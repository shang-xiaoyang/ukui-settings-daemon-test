QT += core gui dbus
CONFIG += c++11 no_keywords link_pkgconfig x11extras
CONFIG -= app_bundle

INCLUDEPATH += -I $$PWD/

PLUGIN_INSTALL_DIRS = /usr/lib/ukui-settings-daemon

PKGCONFIG += \
        glib-2.0\
        gio-2.0 libxklavier \
        x11 xrandr xtst

SOURCES += \
    $$PWD/clib-syslog.c\
    $$PWD/QGSettings/qconftype.cpp\
    $$PWD/QGSettings/qgsettings.cpp \
    $$PWD/xeventmonitor.cpp

HEADERS += \
    $$PWD/clib-syslog.h \
    $$PWD/plugin-interface.h\
    $$PWD/QGSettings/qconftype.h\
    $$PWD/QGSettings/qgsettings.h \
    $$PWD/xeventmonitor.h
