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

#include <QCoreApplication>
#include <QFileInfo>
#include <QNetworkReply>
#include <QUrlQuery>

namespace atools {
namespace util {

HttpDownloader::HttpDownloader(QObject *parent, bool verboseLogging)
  : QObject(parent), verbose(verboseLogging)
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
    qDebug() << Q_FUNC_INFO << downloadUrl << userAgent << acceptEncoding;

  if(downloadUrl.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "URL is empty. Download suspended.";
    return;
  }

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
    // Load a file ================================================================
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
    // Start download ================================================================
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
      // Either cancel requested, request in process or no periodic downloads done
      if(restartRequest || !isDownloading() || updatePeriodSeconds == 0)
      {
        cancelDownload();

        QNetworkRequest request(downloadUrl);

        if(!userAgent.isEmpty())
          request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

        // curl -H "Accept-Encoding: gzip" https://data.vatsim.net/v3/vatsim-data.json --output vatsim-data.json.gz
        if(!acceptEncoding.isEmpty())
          request.setRawHeader(QByteArray("Accept-Encoding"), acceptEncoding.toUtf8());

        // Add arbitrary headers ===================
        for(auto it = headerParameters.begin(); it != headerParameters.end(); ++it)
          request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());

        if(!postParameters.isEmpty())
          // Post raw data ============================
          reply = networkManager.post(request, postParameters);
        else if(!postParametersQuery.isEmpty())
        {
          // Post form data ============================
          request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

          QUrlQuery params;
          for(auto it = postParametersQuery.begin(); it != postParametersQuery.end(); ++it)
            params.addQueryItem(it.key(), it.value());

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
          connect(reply, &QNetworkReply::sslErrors, this, &HttpDownloader::sslErrors);
        }
        else
          qWarning() << Q_FUNC_INFO << "Reply is null" << downloadUrl;
      }
      else
        // is already downloading and waiting for finished required (restartRequest = false)
        startTimer();
    }
  }
}

void HttpDownloader::sslErrors(const QList<QSslError>& errors)
{
  if(reply != nullptr)
  {
    if(!ignoreSslErrors)
    {
      if(!sslErrorLogged)
      {
        qWarning() << Q_FUNC_INFO << errors << reply->url();
        sslErrorLogged = true;
      }

      // Errors not ignored - let user decide if to continue
      QStringList errorList;
      for(QSslError err : errors)
        errorList.append(err.errorString());

      emit downloadSslErrors(errorList, reply->url().toString());
    }

    // ignoreSslErrors set in call above or not

    if(ignoreSslErrors)
      // Continue despite of errors
      reply->ignoreSslErrors();
  }
}

void HttpDownloader::cancelDownload()
{
  stopTimer();
  deleteReply();
  data.clear();
}

void HttpDownloader::setPostParameters(const QStringList& parameters)
{
  postParameters.clear();

  postParametersQuery.clear();
  for(int i = 0; i < parameters.size() - 1; i += 2)
    postParametersQuery.insert(parameters.at(i), parameters.at(i + 1));
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
              arg(QCoreApplication::applicationName()).
              arg(QCoreApplication::applicationVersion()).
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
              arg(QCoreApplication::applicationName()).
              arg(QCoreApplication::applicationVersion()).
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
      qDebug() << Q_FUNC_INFO << "URL" << curUrl() << "error" << reply->error() << reply->rawHeaderPairs();

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
      qWarning() << Q_FUNC_INFO << "URL" << curUrl() << "error" << reply->error();
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
      qWarning() << Q_FUNC_INFO << "URL" << curUrl() << "error" << reply->error();
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
