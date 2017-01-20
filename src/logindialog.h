/*
 * Copyright (C) 2015-2016 Symeon Huang <hzwhuang@gmail.com>
 *
 * shadowsocks-qt5 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-qt5 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libQtShadowsocks; see the file LICENSE. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QNetworkReply>
#include "httpclient.h"
#include "confighelper.h"
#include "mainwindow.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(ConfigHelper* conf, MainWindow* mainwnd, QWidget *parent = 0, bool autoLogin = true);
    ~LoginDialog();

private:
    Ui::LoginDialog *ui;
    HttpClient *http;
    ConfigHelper *conf;
    MainWindow *mainwnd;
    void bootupMainWindow(const QJsonObject &);

signals:
    void logined(const QJsonObject &obj);
    void canceled();

private slots:
    void onAccepted();
    void onCanceled();
    void login();
    void onRequestSuccess(const QJsonObject &obj);
    void onRequestError(const QNetworkReply::NetworkError &err, const QString &errstr);
    void enableInputs();
    void disableInputs();
};

#endif // LOGINDIALOG_H
