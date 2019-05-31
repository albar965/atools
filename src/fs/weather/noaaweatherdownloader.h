/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_NOAAWEATHERDOWNLOADER_H
#define ATOOLS_NOAAWEATHERDOWNLOADER_H

#include <QObject>
#include <QTimer>
#include <functional>

namespace atools {
namespace geo {
class Pos;
}
namespace util {
class HttpDownloader;
}

namespace fs {
namespace weather {

class MetarIndex;
struct MetarResult;

/*
 * Downloads the three latest METAR files from https://tgftp.nws.noaa.gov/data/observations/metar/cycles/00Z.TXT and
 * merges the METAR entries into an index.
 */
class NoaaWeatherDownloader :
  public QObject
{
  Q_OBJECT

public:
  explicit NoaaWeatherDownloader(QObject *parent, int indexSize, bool verboseLogging);
  virtual ~NoaaWeatherDownloader() override;

  /*
   * @return metar from cache or empty if not entry was found in the cache. Once the request was
   * completed the signal weatherUpdated is emitted and calling this method again will return the metar.
   */
  atools::fs::weather::MetarResult getMetar(const QString& airportIcao, const atools::geo::Pos& pos);

  /* Set request base URL. https://tgftp.nws.noaa.gov/data/observations/metar/cycles/%1Z.TXT */
  void setRequestUrl(const QString& url);

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value);

  /* Download files again every given number of seconds */
  void setUpdatePeriod(int seconds);

  /* Download in progress or file downloads outstanding */
  bool isDownloading() const;

  /* Copy airports from the complete list to the index with coordinates.
   * Copies only airports that exist in the current simulator database, i.e. where fetchAirportCoords returns
   * a valid coordinate. */
  void updateIndex();

  /* Append download jobs to the queue and start if not already downloading */
  void startDownload();

signals:
  /* Emitted when file was downloaded and udpated */
  void weatherUpdated();
  void weatherDownloadFailed(const QString& error, int errorCode, QString url);

private:
  void downloadFinished(const QByteArray& data, QString url);
  void downloadFailed(const QString& error, int errorCode, QString url);

  /* Read downloaded METAR file contents */
  bool read(const QByteArray& data, const QString& url);

  /* Start download of next job in the queue */
  void download();

  /* Append a job to the download queue. timeOffset will be substracted from current UTC hour. */
  void appendJob(QDateTime datetime, int timeOffsetHour);
  void startTimer();
  void stopTimer();

  atools::fs::weather::MetarIndex *index = nullptr;
  atools::util::HttpDownloader *downloader = nullptr;

  /* Need to do own updates since more files have to be queued */
  QTimer updateTimer, initialDownloadTimer;

  /* https://tgftp.nws.noaa.gov/data/observations/metar/cycles/%1Z.TXT */
  QString baseUrl;
  bool verbose;
  QStringList downloadQueue;
  int updatePeriodSeconds = 600;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_NOAAWEATHERDOWNLOADER_H
