/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include <QNetworkReply>

namespace atools {
namespace util {

HttpDownloader::HttpDownloader(QObject *parent, bool logVerbose)
  : QObject(parent), verbose(logVerbose)
{
  connect(&updateTimer, &QTimer::timeout, this, &HttpDownloader::startDownload);
}

HttpDownloader::~HttpDownloader()
{
  stopTimer();
  deleteReply();
}

void HttpDownloader::startDownload()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << url;

  cancelDownload();

  QNetworkRequest request((QUrl(url)));

  if(!userAgent.isEmpty())
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

  reply = networkManager.get(request);

  if(reply != nullptr)
  {
    connect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
    connect(reply, &QNetworkReply::readyRead, this, &HttpDownloader::readyRead);
    connect(reply, &QNetworkReply::downloadProgress, this, &HttpDownloader::downloadProgressInternal);
  }
  else
    qWarning() << Q_FUNC_INFO << "Reply is null" << url;
}

void HttpDownloader::cancelDownload()
{
  stopTimer();
  deleteReply();
  data.clear();
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

void HttpDownloader::setUpdatePeriod(int seconds)
{
  updatePeriodSeconds = seconds;
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
  emit downloadProgress(bytesReceived, bytesTotal, reply != nullptr ? reply->url().toString() : QString());
}

void HttpDownloader::httpFinished()
{
  if(reply != nullptr)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << reply->request().url();

    data.append(reply->readAll());

    if(reply->error() == QNetworkReply::NoError)
    {
      emit downloadFinished(data, reply->url().toString());
      deleteReply();
      startTimer();
    }
    else
    {
      emit downloadFailed(reply->errorString(), reply->url().toString());
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
      emit downloadFailed(reply->errorString(), reply->url().toString());
      deleteReply();
      startTimer();
    }
    else
      data.append(reply->read(reply->bytesAvailable()));
  }
}

} // namespace util
} // namespace atools
