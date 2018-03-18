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

HttpDownloader::HttpDownloader(QObject *parent) : QObject(parent)
{
  connect(&updateTimer, &QTimer::timeout, this, &HttpDownloader::startDownload);
}

HttpDownloader::~HttpDownloader()
{
  stopTimer();
  cancelReply();
}

void HttpDownloader::startDownload()
{
  cancelReply();
  data.clear();

  QNetworkRequest request((QUrl(url)));

  if(!userAgent.isEmpty())
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

  reply = networkManager.get(request);

  if(reply != nullptr)
  {
    connect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
    connect(reply, &QNetworkReply::readyRead, this, &HttpDownloader::readyRead);
  }
  else
    qWarning() << Q_FUNC_INFO << "Reply is null" << url;
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

void HttpDownloader::cancelReply()
{
  if(reply != nullptr)
  {
    disconnect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
    reply->abort();
    reply->deleteLater();
    reply = nullptr;
  }
}

void HttpDownloader::httpFinished()
{
  qDebug() << Q_FUNC_INFO;

  if(reply != nullptr)
  {
    data.append(reply->readAll());

    if(reply->error() == QNetworkReply::NoError)
      emit downloadFinished(data);
    else
      emit downloadFailed(reply->errorString());

    cancelReply();
    startTimer();
  }
}

void HttpDownloader::readyRead()
{
  if(reply != nullptr)
  {
    if(reply->error() != QNetworkReply::NoError)
      emit downloadFailed(reply->errorString());

    data.append(reply->read(reply->bytesAvailable()));
  }
}

} // namespace util
} // namespace atools
