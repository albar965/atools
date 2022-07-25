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

#ifndef ATOOLS_HTTPDOWNLOADER_H
#define ATOOLS_HTTPDOWNLOADER_H

#include <QNetworkAccessManager>
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
  explicit HttpDownloader(QObject *parent, bool verboseLogging = false);
  virtual ~HttpDownloader() override;

  HttpDownloader(const HttpDownloader& other) = delete;
  HttpDownloader& operator=(const HttpDownloader& other) = delete;

  /* Download file and emit downloadFinished when done.
   * Will start update timer after download if period <> -1.
   * Cancels all previous downloads if restartRequest = true. */
  void startDownload();
  void cancelDownload();

  /* Set download request URL for GET and POST methods */
  void setUrl(const QString& requestUrl)
  {
    downloadUrl = requestUrl;
  }

  /* Set parameters for POST method. GET is used if empty. Translates to "key1=value1&key2=value2". */
  void setPostParameters(const QHash<QString, QString>& parameters)
  {
    postParameters.clear();
    postParametersQuery = parameters;
  }

  /* Set parameters for POST method. GET is used if empty. Translates to "key1=value1&key2=value2".
   *  List has to contain key/value pairs like key1,value1,key2,value2,... */
  void setPostParameters(const QStringList& parameters);

  /* Set parameters for POST method. GET is used if empty. */
  void setPostParameters(const QByteArray& parameters)
  {
    postParametersQuery.clear();
    postParameters = parameters;
  }

  const QByteArray& getPostParameters() const
  {
    return postParameters;
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
  void setUpdatePeriod(int seconds)
  {
    updatePeriodSeconds = seconds;
  }

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

  /*
   * Sets the user agent based on application name, version and OS.
   * "Little Navmap/1.9.1.develop (Ubuntu 17.10; x86_64; de-DE) Qt 5.9.3 EXTENSION"
   */
  void setDefaultUserAgent(const QString& extension = QString());

  /*
   * Sets the user agent based on application name and version.
   * Short version not revealing too much information about OS.
   * "Little Navmap/1.9.1.develop EXTENSION"
   */
  void setDefaultUserAgentShort(const QString& extension = QString());

  /* true if download is in progress */
  bool isDownloading() const
  {
    return reply != nullptr;
  }

  /* Cancels current request if true. Waits for request to be finished if false */
  void setRestartRequest(bool value)
  {
    restartRequest = value;
  }

  bool isRestartRequest() const
  {
    return restartRequest;
  }

  /* Set to true to ignore any certificate validation or other SSL errors.
   * downloadSslErrors is emitted in case of SSL errors. */
  void setIgnoreSslErrors(bool value)
  {
    ignoreSslErrors = value;
  }

  bool isIgnoreSslErrors() const
  {
    return ignoreSslErrors;
  }

  const QString& getAcceptEncoding() const
  {
    return acceptEncoding;
  }

  /* String parameter to send as "Accept-Encoding" parameter. Set to "gzip" to enable compressed downloads. */
  void setAcceptEncoding(const QString& value)
  {
    acceptEncoding = value;
  }

  /* HTTP header parameters */
  const QHash<QString, QString>& getHeaderParameters() const
  {
    return headerParameters;
  }

  void setHeaderParameters(const QHash<QString, QString>& value)
  {
    headerParameters = value;
  }

signals:
  /* Emitted when file was downloaded and udpated */
  void downloadFinished(const QByteArray& data, QString downloadUrl);
  void downloadFailed(const QString& error, int errorCode, QString downloadUrl);
  void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, QString downloadUrl);

  /* Emitted on SSL errors. Call setIgnoreSslErrors to ignore future errors and continue.  */
  void downloadSslErrors(const QStringList& errors, const QString& downloadUrl);

private:
  /* Request completely finished */
  void httpFinished();

  /* Cancel request and free resources */
  void deleteReply();

  /* A chunk of data is available */
  void readyRead();

  void downloadProgressInternal(qint64 bytesReceived, qint64 bytesTotal);

  void sslErrors(const QList<QSslError>& errors);

  bool restartRequest = true, ignoreSslErrors = false, sslErrorLogged = false;

  QString curUrl();

  QNetworkAccessManager networkManager;
  QTimer updateTimer;
  QString downloadUrl, userAgent, acceptEncoding;

  QByteArray postParameters;
  QHash<QString, QString> postParametersQuery, headerParameters;

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
