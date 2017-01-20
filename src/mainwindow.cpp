#include "defs.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "logindialog.h"

#include "connection.h"
#include "editdialog.h"
#include "logdialog.h"
#include "settingsdialog.h"
#include "sqprofile.h"
#include "versionutil.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QLocalSocket>
#include <botan/version.h>

MainWindow::MainWindow(ConfigHelper *confHelper, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    configHelper(confHelper)
{
    Q_ASSERT(configHelper);

    initSingleInstance();
    heartbeatRequester = new HttpClient(this);
    connect(heartbeatRequester, &HttpClient::success, this, &MainWindow::onHeartBeatResponse);

    ui->setupUi(this);

    //setup Settings menu
#ifndef Q_OS_DARWIN
    ui->menuSettings->addAction(ui->toolBar->toggleViewAction());
#endif

    //initialisation
    model = new ConnectionTableModel(this);
    configHelper->read(model);
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setSortRole(Qt::EditRole);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1);//read from all columns
    ui->connectionView->setModel(proxyModel);
    ui->toolBar->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>
                                    (configHelper->getToolbarStyle()));
    setupActionIcon();

    notifier = new StatusNotifier(this, configHelper->isHideWindowOnStartup(), this);

    connect(configHelper, &ConfigHelper::toolbarStyleChanged,
            ui->toolBar, &QToolBar::setToolButtonStyle);
    connect(model, &ConnectionTableModel::message,
            notifier, &StatusNotifier::showNotification);
    connect(model, &ConnectionTableModel::rowStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
//    connect(ui->actionSaveManually, &QAction::triggered,
//            this, &MainWindow::onSaveManually);
    connect(ui->actionTestAllLatency, &QAction::triggered,
            model, &ConnectionTableModel::testAllLatency);

    //some UI changes accoding to config
    ui->toolBar->setVisible(configHelper->isShowToolbar());
    ui->actionShowFilterBar->setChecked(configHelper->isShowFilterBar());
    ui->menuBar->setNativeMenuBar(configHelper->isNativeMenuBar());

    //Move to the center of the screen
    this->move(QApplication::desktop()->screen()->rect().center() -
               this->rect().center());

    //UI signals
    connect(ui->actionChangeUser, &QAction::triggered, this, &MainWindow::onChangeUser);
    connect(ui->actionQuit, &QAction::triggered, qApp, &QApplication::quit);
    connect(ui->actionConnect, &QAction::triggered,
            this, &MainWindow::onConnect);
    connect(ui->actionForceConnect, &QAction::triggered,
            this, &MainWindow::onForceConnect);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::onDisconnect);
    connect(ui->actionTestLatency, &QAction::triggered,
            this, &MainWindow::onLatencyTest);
    connect(ui->actionViewLog, &QAction::triggered,
            this, &MainWindow::onViewLog);
    connect(ui->actionMoveUp, &QAction::triggered, this, &MainWindow::onMoveUp);
    connect(ui->actionMoveDown, &QAction::triggered,
            this, &MainWindow::onMoveDown);
    connect(ui->actionGeneralSettings, &QAction::triggered,
            this, &MainWindow::onGeneralSettings);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
    connect(ui->actionShowFilterBar, &QAction::toggled,
            configHelper, &ConfigHelper::setShowFilterBar);
    connect(ui->actionShowFilterBar, &QAction::toggled,
            this, &MainWindow::onFilterToggled);
    connect(ui->toolBar, &QToolBar::visibilityChanged,
            configHelper, &ConfigHelper::setShowToolbar);
    connect(ui->filterLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onFilterTextChanged);

    connect(ui->connectionView, &QTableView::clicked,
            this, static_cast<void (MainWindow::*)(const QModelIndex&)>
                  (&MainWindow::checkCurrentIndex));
    connect(ui->connectionView, &QTableView::activated,
            this, static_cast<void (MainWindow::*)(const QModelIndex&)>
                  (&MainWindow::checkCurrentIndex));
//    connect(ui->connectionView, &QTableView::doubleClicked,
//            this, &MainWindow::onEdit);

    /* set custom context menu */
    ui->connectionView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->connectionView, &QTableView::customContextMenuRequested,
            this, &MainWindow::onCustomContextMenuRequested);

    checkCurrentIndex();

    // Restore mainWindow's geometry and state
    restoreGeometry(configHelper->getMainWindowGeometry());
    restoreState(configHelper->getMainWindowState());
    ui->connectionView->horizontalHeader()->restoreGeometry(configHelper->getTableGeometry());
    ui->connectionView->horizontalHeader()->restoreState(configHelper->getTableState());

    // Set column widths
    ui->connectionView->setColumnWidth(0, 150);
    ui->connectionView->setColumnWidth(4, 200);

    // Set up heartbeat timer
    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, &MainWindow::onHeartBeat);
}

MainWindow::~MainWindow()
{
    configHelper->save(*model);
    configHelper->setTableGeometry(ui->connectionView->horizontalHeader()->saveGeometry());
    configHelper->setTableState(ui->connectionView->horizontalHeader()->saveState());
    configHelper->setMainWindowGeometry(saveGeometry());
    configHelper->setMainWindowState(saveState());

    // delete ui after everything in case it's deleted while still needed for
    // the functions written above
    delete ui;
}

const QUrl MainWindow::issueUrl =
        QUrl("https://github.com/shadowsocks/shadowsocks-qt5/issues");


void MainWindow::setUserInfo(const QJsonObject &info)
{
    userInfo = new QJsonObject(info);
    this->updateDisplayServers();
}

void MainWindow::setSessionId(const QString &id)
{
    sessionId = id;
}

void MainWindow::startHeartBeat()
{
    heartbeatTimer->start(1000 * RALLETS_HEARTBEAT_INTERVAL);
    // heartbeat for the first time
    this->onHeartBeat();
}

bool profileIdentical(SQProfile a, SQProfile b) {
    return a.serverAddress == b.serverAddress && a.serverPort == b.serverPort
           && a.method == b.method && a.password == b.password && a.localPort == b.localPort;
}

void MainWindow::updateDisplayServers()
{
    //model->removeRows(0, model->rowCount());
    QJsonArray serverList = ((*userInfo)["self"].toObject())["ssconfigs"].toArray();
    QString lastConnectServerId = configHelper->getValue("LastConnectServerId", "").toString();

    QVector<SQProfile> remoteList;

    for(auto i = serverList.begin(); i != serverList.end(); i++)
    {
        QJsonObject currServer = (*i).toObject();
        //qDebug() << currServer;
        SQProfile p;
        p.localAddress = "127.0.0.1";
        p.id = currServer["id"].toString();
        p.localPort = currServer["port"].toInt();
        p.method = currServer["method"].toString();
        p.name = currServer["remarks"].toString();
        p.password = currServer["password"].toString();
        p.serverAddress = currServer["server"].toString();
        p.serverPort = currServer["server_port"].toString().toInt();

        remoteList.append(p);

        // find if the server is already there
        bool isthere = false;
        int size = model->rowCount();
        for (int j = 0; j < size; ++j) {
            Connection *con = model->getItem(j)->getConnection();
            SQProfile tmpp = con->getProfile();
            if (profileIdentical(tmpp, p)) {
                isthere = true;
                break;
            }
        }

        if (isthere) continue;

        Connection * newconn = new Connection(p, this);
        model->appendConnection(newconn);
    }

    // check if some server has to be removed
    QVector<Connection *> toBeRemoved;
    int size = model->rowCount();
    for (int i = 0; i < size; ++i) {
        Connection *con = model->getItem(i)->getConnection();
        SQProfile tmpp = con->getProfile();
        bool found = false;
        for (int i = 0; i < remoteList.size(); ++i) {
            SQProfile p = remoteList.at(i);
            if (profileIdentical(p, tmpp)) {
                found = true;
                break;
            }
        }
        if (!found) {
            toBeRemoved.append(con);
        }
    }
    // do remove
    for (int i = 0; i < toBeRemoved.size(); ++i) {
        int currSize = model->rowCount();
        for (int j = 0; j < currSize; ++j) {
            Connection *con = model->getItem(j)->getConnection();
            if (con->isRunning()) {
                qDebug() << "stopped tobe removed connection";
                con->stop();
            }
            if (toBeRemoved.at(i) == con) {
                qDebug() << "removing" << j;
                model->removeRow(j);
                break;
            }
        }
    }

    bool connected = false;
    size = model->rowCount();
    for (int i = 0; i < size; ++i) {
        Connection *con = model->getItem(i)->getConnection();
        if (con->isRunning()) {
            connected = true;
            break;
        } else {
            if (con->getProfile().id == lastConnectServerId) {
                stopCurrentConnection();
                con->start();
                qDebug() << "started last connected connection";
                connected = true;
                break;
            }
        }
    }

    // if no server connected, connect a random one
    if (!connected && model->rowCount() > 0)
    {
        qDebug() << "started random connection";
        model->getItem(0)->getConnection()->start();
    }

    model->testAllLatency();
}

void MainWindow::startAutoStartConnections()
{
    configHelper->startAllAutoStart(*model);
}

void MainWindow::stopCurrentConnection()
{
    int size = model->rowCount();
    for (int i = 0; i < size; ++i) {
        Connection *con = model->getItem(i)->getConnection();
        con->stop();
    }
}

void MainWindow::clearConnectionList()
{
    model->removeRows(0, model->rowCount());
}

void MainWindow::onChangeUser()
{
    stopCurrentConnection();
    clearConnectionList();
    heartbeatTimer->stop();
    userInfo = NULL;
    LoginDialog *login = new LoginDialog(configHelper, this, NULL, false);
    login->show();
    this->hide();
}

void MainWindow::onImportGuiJson()
{
    QString file = QFileDialog::getOpenFileName(
                   this,
                   tr("Import Connections from gui-config.json"),
                   QString(),
                   "GUI Configuration (gui-config.json)");
    if (!file.isNull()) {
        configHelper->importGuiConfigJson(model, file);
    }
}

void MainWindow::onExportGuiJson()
{
    QString file = QFileDialog::getSaveFileName(
                   this,
                   tr("Export Connections as gui-config.json"),
                   QString("gui-config.json"),
                   "GUI Configuration (gui-config.json)");
    if (!file.isNull()) {
        configHelper->exportGuiConfigJson(*model, file);
    }
}

void MainWindow::onSaveManually()
{
    configHelper->save(*model);
}

void MainWindow::onAddManually()
{
    Connection *newCon = new Connection;
    newProfile(newCon);
}

void MainWindow::onAddFromConfigJSON()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open config.json"),
                                                QString(), "JSON (*.json)");
    if (!file.isNull()) {
        Connection *con = configHelper->configJsonToConnection(file);
        if (con) {
            newProfile(con);
        }
    }
}

void MainWindow::onDelete()
{
    if (model->removeRow(proxyModel->mapToSource(
                         ui->connectionView->currentIndex()).row())) {
        configHelper->save(*model);
    }
    checkCurrentIndex();
}

void MainWindow::onEdit()
{
    editRow(proxyModel->mapToSource(ui->connectionView->currentIndex()).row());
}

void MainWindow::onConnect()
{
    int row = proxyModel->mapToSource(ui->connectionView->currentIndex()).row();
    Connection *con = model->getItem(row)->getConnection();
    QString serverId = con->getProfile().id;
    configHelper->setValue("LastConnectServerId", serverId);
    if (con->isValid()) {
        model->disconnectConnectionsAt(con->getProfile().localAddress,
                                       con->getProfile().localPort);
        con->start();
    } else {
        QMessageBox::critical(this, tr("Invalid"),
                              tr("The connection's profile is invalid!"));
    }
}

void MainWindow::onForceConnect()
{
    onConnect();
}

void MainWindow::onDisconnect()
{
    int row = proxyModel->mapToSource(ui->connectionView->currentIndex()).row();
    model->getItem(row)->getConnection()->stop();
}

void MainWindow::onConnectionStatusChanged(const int row, const bool running)
{
    if (proxyModel->mapToSource(
                ui->connectionView->currentIndex()).row() == row) {
        ui->actionConnect->setEnabled(!running);
        ui->actionDisconnect->setEnabled(running);
    }
}

void MainWindow::onLatencyTest()
{
    model->getItem(proxyModel->mapToSource(ui->connectionView->currentIndex()).
                   row())->testLatency();
}

void MainWindow::onViewLog()
{
    Connection *con = model->getItem(
                proxyModel->mapToSource(ui->connectionView->currentIndex()).
                row())->getConnection();
    LogDialog *logDlg = new LogDialog(con, this);
    connect(logDlg, &LogDialog::finished, logDlg, &LogDialog::deleteLater);
    logDlg->exec();
}

void MainWindow::onMoveUp()
{
    QModelIndex proxyIndex = ui->connectionView->currentIndex();
    int currentRow = proxyModel->mapToSource(proxyIndex).row();
    int targetRow = proxyModel->mapToSource(
                proxyModel->index(proxyIndex.row() - 1,
                                  proxyIndex.column(),
                                  proxyIndex.parent())
                                           ).row();
    model->move(currentRow, targetRow);
    checkCurrentIndex();
}

void MainWindow::onMoveDown()
{
    QModelIndex proxyIndex = ui->connectionView->currentIndex();
    int currentRow = proxyModel->mapToSource(proxyIndex).row();
    int targetRow = proxyModel->mapToSource(
                proxyModel->index(proxyIndex.row() + 1,
                                  proxyIndex.column(),
                                  proxyIndex.parent())
                                           ).row();
    model->move(currentRow, targetRow);
    checkCurrentIndex();
}

void MainWindow::onGeneralSettings()
{
    SettingsDialog *sDlg = new SettingsDialog(configHelper, this);
    connect(sDlg, &SettingsDialog::finished,
            sDlg, &SettingsDialog::deleteLater);
    if (sDlg->exec()) {
        configHelper->save(*model);
        configHelper->setStartAtLogin();
    }
}

void MainWindow::newProfile(Connection *newCon)
{
    EditDialog *editDlg = new EditDialog(newCon, this);
    connect(editDlg, &EditDialog::finished, editDlg, &EditDialog::deleteLater);
    if (editDlg->exec()) {//accepted
        model->appendConnection(newCon);
        configHelper->save(*model);
    } else {
        newCon->deleteLater();
    }
}

void MainWindow::editRow(int row)
{
    Connection *con = model->getItem(row)->getConnection();
    EditDialog *editDlg = new EditDialog(con, this);
    connect(editDlg, &EditDialog::finished, editDlg, &EditDialog::deleteLater);
    if (editDlg->exec()) {
        configHelper->save(*model);
    }
}

void MainWindow::checkCurrentIndex()
{
    checkCurrentIndex(ui->connectionView->currentIndex());
}

void MainWindow::checkCurrentIndex(const QModelIndex &_index)
{
    QModelIndex index = proxyModel->mapToSource(_index);
    const bool valid = index.isValid();
    ui->actionTestLatency->setEnabled(valid);
    ui->actionViewLog->setEnabled(valid);
    ui->actionMoveUp->setEnabled(valid ? _index.row() > 0 : false);
    ui->actionMoveDown->setEnabled(valid ?
                                   _index.row() < model->rowCount() - 1 :
                                   false);

    if (valid) {
        const bool &running =
                model->getItem(index.row())->getConnection()->isRunning();
        ui->actionConnect->setEnabled(!running);
        ui->actionForceConnect->setEnabled(!running);
        ui->actionDisconnect->setEnabled(running);
    } else {
        ui->actionConnect->setEnabled(false);
        ui->actionForceConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
    }
}

void MainWindow::onAbout()
{
    QString text = QString("<h1>Rallets</h1><p><b>Version %1</b><br />"
            "Using Botan %3.%4.%5</p>"
            "<p>Copyright © 2016-2017 Rallets "
            "(<a href='https://rallets.com'>"
            "@rallets</a>)</p>")
            .arg(QStringLiteral(APP_VERSION))
            .arg(QSS::Common::version().data())
            .arg(Botan::version_major())
            .arg(Botan::version_minor())
            .arg(Botan::version_patch());
    QMessageBox::about(this, tr("About"), text);
}

void MainWindow::onReportBug()
{
    QDesktopServices::openUrl(issueUrl);
}

void MainWindow::onCustomContextMenuRequested(const QPoint &pos)
{
    this->checkCurrentIndex(ui->connectionView->indexAt(pos));
    ui->menuConnection->popup(ui->connectionView->viewport()->mapToGlobal(pos));
}

void MainWindow::onFilterToggled(bool show)
{
    if (show) {
        ui->filterLineEdit->setFocus();
    }
}

void MainWindow::onFilterTextChanged(const QString &text)
{
    proxyModel->setFilterWildcard(text);
}

void MainWindow::hideEvent(QHideEvent *e)
{
    QMainWindow::hideEvent(e);
    notifier->onWindowVisibleChanged(false);
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    notifier->onWindowVisibleChanged(true);
    this->setFocus();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (e->spontaneous()) {
        e->ignore();
        hide();
    } else {
        QMainWindow::closeEvent(e);
    }
}

void MainWindow::setupActionIcon()
{
    ui->actionConnect->setIcon(QIcon::fromTheme("network-connect",
                               QIcon::fromTheme("network-vpn")));
    ui->actionDisconnect->setIcon(QIcon::fromTheme("network-disconnect",
                                  QIcon::fromTheme("network-offline")));
    ui->actionTestLatency->setIcon(QIcon::fromTheme("flag",
                                   QIcon::fromTheme("starred")));
    ui->actionViewLog->setIcon(QIcon::fromTheme("view-list-text",
                               QIcon::fromTheme("text-x-preview")));
    ui->actionGeneralSettings->setIcon(QIcon::fromTheme("configure",
                                       QIcon::fromTheme("preferences-desktop")));
}

bool MainWindow::isInstanceRunning() const
{
    return instanceRunning;
}

void MainWindow::initSingleInstance()
{
    instanceRunning = false;

    QString username = qgetenv("USER");
    if (username.isEmpty()) {
        username = qgetenv("USERNAME");
    }

    QString serverName = QCoreApplication::applicationName() + "_" + username;
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        instanceRunning = true;
        if (configHelper->isOnlyOneInstance()) {
            qWarning() << "A instance from the same user is already running";
        }
        socket.write(serverName.toUtf8());
        socket.waitForBytesWritten();
        return;
    }

    /* Cann't connect to server, indicating it's the first instance of the user */
    instanceServer = new QLocalServer(this);
    instanceServer->setSocketOptions(QLocalServer::UserAccessOption);
    connect(instanceServer, &QLocalServer::newConnection,
            this,&MainWindow::onSingleInstanceConnect);
    if (instanceServer->listen(serverName)) {
        /* Remove server in case of crashes */
        if (instanceServer->serverError() == QAbstractSocket::AddressInUseError &&
                QFile::exists(instanceServer->serverName())) {
            QFile::remove(instanceServer->serverName());
            instanceServer->listen(serverName);
        }
    }
}

void MainWindow::onSingleInstanceConnect()
{
    QLocalSocket *socket = instanceServer->nextPendingConnection();
    if (!socket) {
        return;
    }

    if (socket->waitForReadyRead(1000)) {
        QString username = qgetenv("USER");
        if (username.isEmpty()) {
            username = qgetenv("USERNAME");
        }

        QByteArray byteArray = socket->readAll();
        QString magic(byteArray);
        if (magic == QCoreApplication::applicationName() + "_" + username) {
            show();
        }
    }
    delete socket;
}

void MainWindow::onHeartBeat()
{
    qDebug() << "query notification";
    QString query = QString("session_id=" + sessionId + "&DEVICE_TYPE=LINUX&VERSION=" + APP_VERSION);
    heartbeatRequester->post(QString(RALLETS_API_HOST) + "/rallets_notification", query);
}

void MainWindow::onHeartBeatResponse(const QJsonObject &obj)
{
    //qDebug() << "hearbeat result" << obj;
    // heartbeat failed
    if (!obj["ok"].toBool()) {
        stopCurrentConnection();
        clearConnectionList();
        heartbeatTimer->stop();
        userInfo = NULL;
        QMessageBox::warning(this, tr("Main"), obj["message"].toString());
        LoginDialog *login = new LoginDialog(configHelper, this, NULL, false);
        login->show();
        this->close();
        return;
    }

    this->setUserInfo(obj);

    // process notification
    QJsonObject notif = obj["systemNotification"].toObject();
    if (notif.empty())
        return;
    Version localVer = Version(APP_VERSION);
    Version remoteVer = Version(notif["version"].toString().toStdString());
    if (!newVersionAsked && localVer < remoteVer) {
        if (QMessageBox::question(this, tr("Rallets"), tr("A new version is available"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
                == QMessageBox::Yes)
        {
            QString downloadLink = notif["download_link"].toString();
            QDesktopServices::openUrl(QUrl(downloadLink));
        }
        newVersionAsked = true;
    }
    if (notif["show"].toBool()) {
        QMessageBox::information(this, tr("Rallets"), notif["message"].toString(), QMessageBox::Ok, QMessageBox::Ok);
        QString link = notif["link"].toString();
        if (link.startsWith("http")) {
            QDesktopServices::openUrl(QUrl(link));
        }
    }
}
