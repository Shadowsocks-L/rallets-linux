#-------------------------------------------------
#
#          Project Shadowsocks-Qt5
#
#-------------------------------------------------

QT               += core gui widgets network
unix: !macx: QT  += dbus

CONFIG    += c++11

TARGET     = rallets
TEMPLATE   = app
VERSION    = 1.1.0
DEFINES   += APP_VERSION=\\\"$$VERSION\\\"
DEFINES   += QSS_VERSION=\\\"$$VERSION\\\"

include(src/ss-qt5.pri)
include(libqtss.pri)

OTHER_FILES  += README.md \
                rallets.desktop

desktop.files = rallets.desktop
ssicon.files  = src/icons/rallets.png

isEmpty(INSTALL_PREFIX) {
    unix: INSTALL_PREFIX = /usr
    else: INSTALL_PREFIX = ..
}

macx: {
    ICON     = src/icons/shadowsocks-qt5.icns
    CONFIG  += link_pkgconfig
}
unix: {
    desktop.path  = $$INSTALL_PREFIX/share/applications
    ssicon.path   = $$INSTALL_PREFIX/share/icons/hicolor/512x512/apps
    INSTALLS     += desktop ssicon
}
win32: DEFINES   += QSS_STATIC

target.path       = $$INSTALL_PREFIX/bin

INSTALLS         += target
