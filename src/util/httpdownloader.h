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

#ifndef ATOOLS_HTTPDOWNLOADER_H
#define ATOOLS_HTTPDOWNLOADER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

class QNetworkReply;

namespace atools {
namespace util {

template<typename KEY, typename TYPE>
class TimedCache;

/*
 * Simple async HTTP download tool that reads files from web addresses.
 * Has a timer to do recurring downloads and can use a timed cache.
 */
class HttpDownloader :
  public QObject
{
  Q_OBJECT

public:
  HttpDownloader(QObject *parent, bool logVerbose = false);
  virtual ~HttpDownloader();

  /* Download file and emit downloadFinished when done.
   * Will start update timer after download if period <> -1.
   * Cancels all previous downloads. */
  void startDownload();
  void cancelDownload();

  /* Set download request URL */
  void setUrl(const QString& requestUrl)
  {
    downloadUrl = requestUrl;
  }

  /* Enable an internal cache for each request URL */
  void enableCache(int secondsTimeout);

  /* Disable and clear cache*/
  void disableCache();

  const QString& getUrl() const
  {
    return downloadUrl;
  }

  /* Re-download every number of seconds and emit downloadFinished when done.
   *  Timer will start after first reply from startDownload */
  void setUpdatePeriod(int seconds);

  void startTimer();
  void stopTimer();

  /* Is valid until next startDownload call */
  const QByteArray& getData() const
  {
    return data;
  }

  const QString& getUserAgent() const
  {
    return userAgent;
  }

  /* Set to empty string to use Qt default */
  void setUserAgent(const QString& value)
  {
    userAgent = value;
  }

  /* Sets the user agent based on application name, version and OS. */
  void setUserAgent();

signals:
  /* Emitted when file was downloaded and udpated */
  void downloadFinished(const QByteArray& data, QString downloadUrl);
  void downloadFailed(const QString& error, QString downloadUrl);
  void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, QString downloadUrl);

private:
  /* Request completely finished */
  void httpFinished();

  /* Cancel request and free resources */
  void deleteReply();

  /* A chunk of data is available */
  void readyRead();

  void downloadProgressInternal(qint64 bytesReceived, qint64 bytesTotal);

  QString curUrl();

  QNetworkAccessManager networkManager;
  QTimer updateTimer;
  QString downloadUrl, userAgent;
  int updatePeriodSeconds = -1;
  QNetworkReply *reply = nullptr;
  QByteArray data;
  bool verbose;

  /* Maps URL to result */
  atools::util::TimedCache<QString, QByteArray> *dataCache = nullptr;

};

} // namespace util
} // namespace atools

#endif // ATOOLS_HTTPDOWNLOADER_H
