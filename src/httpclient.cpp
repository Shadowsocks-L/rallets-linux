#include "httpclient.h"
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

HttpClient::HttpClient(QObject *parent) :
    QObject(parent)
{
    connect(&qnam, &QNetworkAccessManager::finished, this, &HttpClient::onResponse);
    connect(&qnam, &QNetworkAccessManager::sslErrors, this, &HttpClient::sslErrors);
}

HttpClient::~HttpClient()
{
}

void HttpClient::post(const QString & urlstr, const QString & postfields)
{
    QUrl url(urlstr);
    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    qnam.post(req, postfields.toUtf8());
}

void HttpClient::get(const QString & urlstr)
{
    QUrl url(urlstr);
    QNetworkRequest req(url);
    qnam.get(req);
}

void HttpClient::onResponse(QNetworkReply *reply)
{
  if(reply->error() == QNetworkReply::NoError)
  {
      QByteArray bytes = reply->readAll();
      QString result(bytes);
      QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
      QJsonObject obj = doc.object();

      emit success(obj);
  }
  else {
      qDebug() << "reply with error:  " << reply->error() << " " << reply->errorString();
      emit error(reply->error(), reply->errorString());
  }
}

void HttpClient::sslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    // ignore self signed ssl error
    reply->ignoreSslErrors();
}

