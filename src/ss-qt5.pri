SOURCES      += src/main.cpp\
                src/mainwindow.cpp \
                src/ip4validator.cpp \
                src/portvalidator.cpp \
                src/ssvalidator.cpp \
                src/editdialog.cpp \
                src/logdialog.cpp \
                src/connection.cpp \
                src/confighelper.cpp \
                src/sqprofile.cpp \
                src/settingsdialog.cpp \
                src/logindialog.cpp \
                src/statusnotifier.cpp \
                src/connectiontablemodel.cpp \
                src/connectionitem.cpp \
                src/httpclient.cpp

HEADERS      += src/mainwindow.h \
                src/ip4validator.h \
                src/portvalidator.h \
                src/ssvalidator.h \
                src/editdialog.h \
                src/logdialog.h \
                src/connection.h \
                src/confighelper.h \
                src/sqprofile.h \
                src/settingsdialog.h \
                src/logindialog.h \
                src/statusnotifier.h \
                src/connectiontablemodel.h \
                src/connectionitem.h \
                src/httpclient.h \
                src/defs.h \
    $$PWD/versionutil.h

FORMS        += src/mainwindow.ui \
                src/editdialog.ui \
                src/logdialog.ui \
                src/settingsdialog.ui \
                src/logindialog.ui

RESOURCES    += src/icons.qrc \
                src/translations.qrc

TRANSLATIONS += src/i18n/ss-qt5_zh_CN.ts \
                src/i18n/ss-qt5_zh_TW.ts

win32: RC_FILE = src/ss-qt5.rc

isEmpty(BOTAN_VER) {
    BOTAN_VER = 1.10
}

win32: {
    win32-msvc*: error("Doesn't Support MSVC! Please use MinGW GCC.")
    else: {
        INCLUDEPATH += D:/Projects/libQtShadowsocks/lib#just for convenience
    }
    LIBS += -L./ -lQtShadowsocks -lbotan-$$BOTAN_VER -liconv
}
macx: {
    QT_CONFIG -= no-pkg-config
}
unix: {
    CONFIG    += link_pkgconfig
    PKGCONFIG += botan-$$BOTAN_VER
    !macx: {
        PKGCONFIG += gtk+-2.0 appindicator-0.1
        DEFINES   += USE_APP_INDICATOR
    }
}
