#-------------------------------------------------
#
# Project created by QtCreator 2020-03-17T09:30:00
#
#-------------------------------------------------
OTHER_FILES += \
    $$PWD/ukui-settings-daemon.desktop\
    $$PWD/org.ukui.SettingsDaemon.service\
    $$PWD/org.ukui.font-rendering.gschema.xml.in \
\#    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-settings.gschema.xml \
\#    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-keyboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.background.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.clipboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.housekeeping.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.keybindings.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.keyboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.mpris.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.mouse.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.media-keys.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.sound.gschema.xml \
\#    $$PWD/org.ukui.SettingsDaemon.plugins.smartcard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.xrandr.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.xrdb.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.xsettings.gschema.xml  \
    $$PWD/org.ukui.peripherals-keyboard.gschema.xml	\
    $$PWD/org.ukui.peripherals-mouse.gschema.xml    \
    $$PWD/org.ukui.peripherals-smartcard.gschema.xml     \
    $$PWD/org.ukui.peripherals-touchpad.gschema.xml.in \
    $$PWD/org.ukui.peripherals-touchscreen.gschema.xml.in \

# desktop ok
desktop.path = /etc/xdg/autostart/
desktop.files = $$PWD/ukui-settings-daemon.desktop

plugin_info.path = /usr/local/lib/ukui-settings-daemon/
plugin_info.files = $$PWD/*.ukui-settings-plugin

plugin_schema.path = /usr/share/glib-2.0/schemas/
plugin_schema.files = $$PWD/org.ukui.*.gschema.xml

# dbus
ukui_daemon_dbus.path = /usr/share/dbus-1/services/
ukui_daemon_dbus.files = $$PWD/org.ukui.SettingsDaemon.service

INSTALLS += desktop plugin_info plugin_schema ukui_daemon_dbus

DISTFILES += \
\#    $$PWD/a11y-settings.ukui-settings-plugin \
\#    $$PWD/a11y-keyboard.ukui-settings-plugin \
    $$PWD/background.ukui-settings-plugin \
    $$PWD/clipboard.ukui-settings-plugin     \
    $$PWD/housekeeping.ukui-settings-plugin  \
    $$PWD/media-keys.ukui-settings-plugin    \
    $$PWD/mouse.ukui-settings-plugin         \
    $$PWD/mpris.ukui-settings-plugin         \
    $$PWD/keyboard.ukui-settings-plugin      \
    $$PWD/keybindings.ukui-settings-plugin   \
    $$PWD/sound.ukui-settings-plugin         \
    $$PWD/xrdb.ukui-settings-plugin          \
    $$PWD/xrandr.ukui-settings-plugin       \
    $$PWD/xsettings.ukui-settings-plugin 
#    $$PWD/smartcard.ukui-settings-plugin    \
