#include "defs.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "ui_logindialog.h"
#include "confighelper.h"
#include <QPushButton>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

LoginDialog::LoginDialog(ConfigHelper* conf, MainWindow* mainwnd, QWidget *parent, bool autoLogin) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    this->conf = conf;
    this->mainwnd = mainwnd;
    http = new HttpClient(this);

    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LoginDialog::onAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &LoginDialog::onCanceled);
    //connect(ui->loginEdit, &QLineEdit::textChanged, this, &LoginDialog::onURIChanged);
    connect(http, &HttpClient::success, this, &LoginDialog::onRequestSuccess);
    connect(http, &HttpClient::error, this, &LoginDialog::onRequestError);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Login"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    this->adjustSize();

    QString lastLoginUser = conf->getValue("LastLoginUser", "").toString();
    QString password = conf->getValue("Password", "").toString();
    ui->loginEdit->setText(lastLoginUser);
    ui->passwordEdit->setText(password);
    if (!lastLoginUser.isEmpty())
    {
        ui->passwordEdit->setFocus();
    }

    // show banner
    QGraphicsScene *scene = new QGraphicsScene();
    QPixmap pixmap(":/icons/icons/rallets.png");
    scene->addPixmap(pixmap.scaled(150, 150));
    ui->graphicsView->setStyleSheet("background: transparent; border-style: none;");
    ui->graphicsView->setScene(scene);

    if (!lastLoginUser.isEmpty() && !password.isEmpty() && autoLogin)
    {
        login();
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::login()
{
    disableInputs();

    http->post(QString(RALLETS_API_HOST) + "/login",
              QString("username_or_email=" + ui->loginEdit->text() + "&login_password=" + ui->passwordEdit->text() + "&DEVICE_TYPE=LINUX"));
    qDebug() << "login...";
}

void LoginDialog::onRequestSuccess(const QJsonObject &obj)
{
    enableInputs();

    //qDebug() << "login result" << obj;
    // login failed
    if (!obj["ok"].toBool()) {
        conf->setValue("Password", "");
        QMessageBox::warning(this, tr("Login"), obj["message"].toString());
        return;
    }

    // save password
    conf->setValue("Password", ui->passwordEdit->text());

    // login success
    emit logined(obj);
    this->bootupMainWindow(obj);
}

void LoginDialog::onRequestError(const QNetworkReply::NetworkError &err, const QString &errstr)
{
    enableInputs();
    conf->setValue("Password", "");
    QMessageBox::warning(this, tr("Login"), tr("Login failed"));
}

void LoginDialog::enableInputs()
{
    ui->loginEdit->setEnabled(true);
    ui->passwordEdit->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void LoginDialog::disableInputs()
{
    ui->loginEdit->setEnabled(false);
    ui->passwordEdit->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void LoginDialog::onAccepted()
{
    // save last login user
    conf->setValue("LastLoginUser", ui->loginEdit->text());

    this->login();
}

void LoginDialog::onCanceled()
{
    qDebug() << "user canceled login";
    emit canceled();
    //this->close();
    qApp->quit();
}

void LoginDialog::bootupMainWindow(const QJsonObject &userInfo)
{
    if (!conf->isHideWindowOnStartup()) {
        mainwnd->show();
    }
    mainwnd->setSessionId(userInfo["session_id"].toString());
    mainwnd->startHeartBeat();

    this->close();
}
