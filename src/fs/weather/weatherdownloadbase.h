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

#ifndef ATOOLS_WEATHERDOWNLOADBASE_H
#define ATOOLS_WEATHERDOWNLOADBASE_H

#include "fs/weather/weathertypes.h"

#include <QTimer>

namespace atools {
namespace util {
class HttpDownloader;
}
namespace fs {
namespace weather {

struct MetarResult;
class MetarIndex;

/*
 * Base class for weather downloaders and readers.
 */
class WeatherDownloadBase :
  public QObject
{
  Q_OBJECT

public:
  explicit WeatherDownloadBase(QObject *parent, atools::fs::weather::MetarFormat format, bool verboseLogging);
  virtual ~WeatherDownloadBase() override;

  WeatherDownloadBase(const WeatherDownloadBase& other) = delete;
  WeatherDownloadBase& operator=(const WeatherDownloadBase& other) = delete;

  /*
   * @return metar from cache or empty if not entry was found in the cache. Once the request was
   * completed the signal weatherUpdated is emitted and calling this method again will return the metar.
   *
   * Download and timer is triggered on first call.
   */
  virtual atools::fs::weather::MetarResult getMetar(const QString& airportIcao, const atools::geo::Pos& pos);

  /* Set download request URL */
  virtual void setRequestUrl(const QString& url);
  virtual const QString& getRequestUrl() const;

  /* Download in progress or file downloads outstanding */
  virtual bool isDownloading() const;

  /* Re-download every number of seconds and emit weatherUpdated when done. This will start the update timer. */
  virtual void setUpdatePeriod(int seconds);

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  virtual void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value);

  /* Number of unique METAR entries in the list */
  virtual int size() const;

  /* Set to true to ignore any certificate validation or other SSL errors.
   * downloadSslErrors is emitted in case of SSL errors. */
  void setIgnoreSslErrors(bool value);

  /* Returns true if the error state timer is currently active */
  bool isErrorState() const;

  /* Start a timer that is used to avoid flooding the log with errors.
   * Requests on getMetar() will not trigger a download while this timer is running (three minutes). */
  void setErrorStateTimer(bool error = true);

  /* HTTP header parameters */
  const QHash<QString, QString>& getHeaderParameters() const;
  void setHeaderParameters(const QHash<QString, QString>& value);

  /* Print the size of all container classes to detect overflow or memory leak conditions */
  void debugDumpContainerSizes() const;

signals:
  /* Emitted when file was downloaded and udpated */
  void weatherUpdated();
  void weatherDownloadFailed(const QString& error, int errorCode, QString url);

  /* Emitted on SSL errors. Call setIgnoreSslErrors to ignore future errors and continue.  */
  void weatherDownloadSslErrors(const QStringList& errors, const QString& downloadUrl);

  /* Emitted during download */
  void weatherDownloadProgress(qint64 bytesReceived, qint64 bytesTotal, QString downloadUrl);

protected:
  virtual void startDownload();

  /* Contains all airports that are also available in the current simulator database. */
  atools::fs::weather::MetarIndex *metarIndex;
  atools::util::HttpDownloader *downloader = nullptr;
  bool verbose = false;

  /* Timer that is used to avoid flooding the log with errors. */
  QTimer errorStateTimer;
  static const int ERROR_TIMER_INTERVAL_MS = 3 * 60 * 1000;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_WEATHERDOWNLOADBASE_H
