#-------------------------------------------------
#
# Project created by QtCreator 2020-04-16T09:30:00
#
#-------------------------------------------------
QT -= gui
QT += core widgets

TEMPLATE = lib
TARGET = background

CONFIG += no_keywords c++11 create_prl plugin link_pkgconfig debug

CONFIG += app_bundle

include($$PWD/../../common/common.pri)

INCLUDEPATH += \

PKGCONFIG += \
        gio-2.0 gtk+-3.0 \
        glib-2.0 \
        mate-desktop-2.0

SOURCES += \
        $$PWD/backgroundplugin.cpp \
        background-manager.cpp

HEADERS += \
        $$PWD/backgroundplugin.h \
        background-manager.h

DESTDIR = $$PWD

background_lib.path = $${PLUGIN_INSTALL_DIRS}
background_lib.files += $$PWD/libbackground.so

INSTALLS += background_lib
