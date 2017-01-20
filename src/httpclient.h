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

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QNetworkReply>
#include <QNetworkAccessManager>

class HttpClient : public QObject
{
    Q_OBJECT

public:
    explicit HttpClient(QObject *parent);
    ~HttpClient();
    void post(const QString & urlstr, const QString & postfields);
    void get(const QString & urlstr);

private:
    QNetworkAccessManager qnam;

signals:
    void success(const QJsonObject &obj);
    void error(const QNetworkReply::NetworkError &err, const QString &errstr);

private slots:
    void onResponse(QNetworkReply *reply);
    void sslErrors(QNetworkReply*, const QList<QSslError> &errors);
};

#endif // HTTPCLIENT_H
