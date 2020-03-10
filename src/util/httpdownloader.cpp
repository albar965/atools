/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "util/httpdownloader.h"
#include "util/timedcache.h"

#include <QApplication>
#include <QFileInfo>
#include <QNetworkReply>
#include <QUrlQuery>

namespace atools {
namespace util {

HttpDownloader::HttpDownloader(QObject *parent, bool logVerbose)
  : QObject(parent), verbose(logVerbose)
{
  updateTimer.setSingleShot(true);
  connect(&updateTimer, &QTimer::timeout, this, &HttpDownloader::startDownload);
}

HttpDownloader::~HttpDownloader()
{
  stopTimer();
  deleteReply();
  delete dataCache;
}

void HttpDownloader::startDownload()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << downloadUrl;

  // Check if the URL points to a local file
  QString filename;
  QFileInfo fileinfo(downloadUrl);
  if(fileinfo.exists() && fileinfo.isFile())
    // Is direct path to file
    filename = downloadUrl;
  else
  {
    QUrl url(downloadUrl);
    if(url.isLocalFile())
    {
      // file:// schema
      fileinfo = QFileInfo(url.fileName());
      if(fileinfo.exists() && fileinfo.isFile())
        // Is URL to local file
        filename = downloadUrl;
    }
  }

  if(!filename.isEmpty())
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
      data = file.readAll();
      file.close();

      emit downloadFinished(data, downloadUrl);

      startTimer();
    }
  }
  else
  {
    QByteArray *cachedData = nullptr;
    if(dataCache != nullptr && (cachedData = dataCache->value(QUrl(downloadUrl).toString())) != nullptr)
    {
      // Found value in the cache
      data = *cachedData;
      emit downloadFinished(data, downloadUrl);

      startTimer();
    }
    else
    {
      cancelDownload();

      QNetworkRequest request(downloadUrl);

      if(!userAgent.isEmpty())
        request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

      if(!postParameters.isEmpty())
        // Post raw data ============================
        reply = networkManager.post(request, postParameters);
      else if(!postParametersQuery.isEmpty())
      {
        // Post form data ============================
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QUrlQuery params;
        for(const QString& key : postParametersQuery.keys())
          params.addQueryItem(key, postParametersQuery.value(key));

        reply = networkManager.post(request, params.query().toUtf8());
      }
      else
        // Get request ============================
        reply = networkManager.get(request);

      if(reply != nullptr)
      {
        connect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
        connect(reply, &QNetworkReply::readyRead, this, &HttpDownloader::readyRead);
        connect(reply, &QNetworkReply::downloadProgress, this, &HttpDownloader::downloadProgressInternal);
      }
      else
        qWarning() << Q_FUNC_INFO << "Reply is null" << downloadUrl;
    }
  }
}

void HttpDownloader::cancelDownload()
{
  stopTimer();
  deleteReply();
  data.clear();
}

void HttpDownloader::enableCache(int secondsTimeout)
{
  delete dataCache;
  dataCache = new atools::util::TimedCache<QString, QByteArray>(secondsTimeout);
}

void HttpDownloader::disableCache()
{
  delete dataCache;
  dataCache = nullptr;
}

void HttpDownloader::startTimer()
{
  if(updatePeriodSeconds > 0)
  {
    updateTimer.setInterval(updatePeriodSeconds * 1000);
    updateTimer.start();
  }
  else
    updateTimer.stop();
}

void HttpDownloader::stopTimer()
{
  updateTimer.stop();
}

void HttpDownloader::setDefaultUserAgent(const QString& extension)
{
  // Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:59.0) Gecko/20100101 Firefox/59.0
  // Little Navmap/1.9.1.develop (Ubuntu 17.10; x86_64; de-DE) Qt 5.9.3 extension
  userAgent = QString("%1/%2 (%3; %4; %5) Qt %6%7").
              arg(QApplication::applicationName()).
              arg(QApplication::applicationVersion()).
              arg(QSysInfo::prettyProductName()).
              arg(QSysInfo::buildCpuArchitecture()).
              arg(QLocale().uiLanguages().join("; ")).
              arg(QT_VERSION_STR).
              arg(extension);
}

void HttpDownloader::setDefaultUserAgentShort(const QString& extension)
{
  // Little Navmap/1.9.1.develop extension
  userAgent = QString("%1/%2%3").
              arg(QApplication::applicationName()).
              arg(QApplication::applicationVersion()).
              arg(extension);
}

void HttpDownloader::deleteReply()
{
  if(reply != nullptr)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << reply->request().url();

    disconnect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
    disconnect(reply, &QNetworkReply::readyRead, this, &HttpDownloader::readyRead);
    disconnect(reply, &QNetworkReply::downloadProgress, this, &HttpDownloader::downloadProgressInternal);

    reply->abort();
    reply->deleteLater();
    reply = nullptr;
  }
}

void HttpDownloader::downloadProgressInternal(qint64 bytesReceived, qint64 bytesTotal)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "bytesReceived" << bytesReceived << "bytesTotal" << bytesTotal << "URL" << curUrl();

  emit downloadProgress(bytesReceived, bytesTotal, curUrl());
}

QString HttpDownloader::curUrl()
{
  return reply != nullptr ? reply->url().toString() : QString();
}

void HttpDownloader::httpFinished()
{
  if(reply != nullptr)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "URL" << curUrl();

    data.append(reply->readAll());

    if(reply->error() == QNetworkReply::NoError)
    {
      if(dataCache != nullptr)
        dataCache->insert(reply->url().toString(), data);

      emit downloadFinished(data, reply->url().toString());
      deleteReply();
      startTimer();
    }
    else
    {
      emit downloadFailed(reply->errorString(), reply->error(), curUrl());
      deleteReply();
      startTimer();
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "No reply";
}

void HttpDownloader::readyRead()
{
  if(reply != nullptr)
  {
    if(reply->error() != QNetworkReply::NoError)
    {
      emit downloadFailed(reply->errorString(), reply->error(), curUrl());
      deleteReply();
      startTimer();
    }
    else
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "reply->bytesAvailable()" << reply->bytesAvailable() << "URL" << curUrl();
      data.append(reply->read(reply->bytesAvailable()));
    }
  }
}

} // namespace util
} // namespace atools
