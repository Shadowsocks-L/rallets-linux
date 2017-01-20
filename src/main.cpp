#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSharedMemory>
#include <QDebug>
#include <QDir>
#include <QCommandLineParser>
#include <signal.h>
#include "mainwindow.h"
#include "logindialog.h"
#include "confighelper.h"

MainWindow *mainWindow = nullptr;

static void onSignalRecv(int sig)
{
#ifdef Q_OS_UNIX
    if (sig == SIGUSR1) {
        if (mainWindow) {
            mainWindow->show();
        }
    }
#endif
    if (sig == SIGINT || sig == SIGTERM) qApp->quit();
}

void setupApplication(QApplication &a)
{
    signal(SIGINT, onSignalRecv);
    signal(SIGTERM, onSignalRecv);
#ifdef Q_OS_UNIX
    signal(SIGUSR1, onSignalRecv);
#endif

    a.setApplicationName(QString("rallets"));
    a.setApplicationDisplayName(QString("Rallets"));
    a.setApplicationVersion(APP_VERSION);

#ifdef Q_OS_WIN
    if (QLocale::system().country() == QLocale::China) {
        a.setFont(QFont("Microsoft Yahei", 9, QFont::Normal, false));
    }
    else {
        a.setFont(QFont("Segoe UI", 9, QFont::Normal, false));
    }
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    QIcon::setThemeName("Breeze");
#endif

    QTranslator *ssqt5t = new QTranslator(&a);
    ssqt5t->load(QLocale::system(), "ss-qt5", "_", ":/i18n");
    a.installTranslator(ssqt5t);
}

int main(int argc, char *argv[])
{
    qRegisterMetaTypeStreamOperators<SQProfile>("SQProfile");

    QApplication a(argc, argv);
    setupApplication(a);


#ifdef Q_OS_WIN
    QString configFile = qApp.applicationDirPath() + "/config.ini";
#else
    QDir configDir = QDir::homePath() + "/.config/rallets";
    QString configFile = configDir.absolutePath() + "/config.ini";
    if (!configDir.exists()) {
        configDir.mkpath(configDir.absolutePath());
    }
#endif
    ConfigHelper conf(configFile);

    MainWindow m(&conf);

    if (conf.isOnlyOneInstance() && m.isInstanceRunning()) {
        return -1;
    }

    LoginDialog login(&conf, &m);
    login.show();

    a.setQuitOnLastWindowClosed(false);

    return a.exec();
}
