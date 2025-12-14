/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/weather/weatherdownloadbase.h"

#include <QTimer>

namespace atools {
namespace geo {
class Pos;
}
namespace util {
class HttpDownloader;
}

namespace fs {
namespace weather {

/*
 * Downloads the three latest METAR files from https://tgftp.nws.noaa.gov/data/observations/metar/cycles/00Z.TXT and
 * merges the METAR entries into an index.
 */
class NoaaWeatherDownloader :
  public WeatherDownloadBase
{
  Q_OBJECT

public:
  explicit NoaaWeatherDownloader(QObject *parent, bool verbose);
  virtual ~NoaaWeatherDownloader() override;

  virtual void setRequestUrl(const QString& url) override;
  virtual bool isDownloading() const override;

  /* Do not set update period in HttpDownloader. NOAA has to maintain its own timer since three
   * files have to be downloaded */
  virtual void setUpdatePeriod(int seconds) override;

private:
  /* Append download jobs to the queue and start if not already downloading */
  virtual void startDownload() override;

  void downloadFinished(const QByteArray& data, QString url);
  void downloadFailed(const QString& error, int errorCode, QString url);

  /* Read downloaded METAR file contents */
  bool read(const QByteArray& data, const QString& url);

  /* Start download of next job in the queue */
  void download();

  /* Append a job to the download queue. timeOffset will be subtracting from current UTC hour. */
  void appendJob(QDateTime datetime, int timeOffsetHour);
  void startTimer();
  void stopTimer();

  /* Need to do own updates since more files have to be queued */
  QTimer updateTimer;

  /* https://tgftp.nws.noaa.gov/data/observations/metar/cycles/%1Z.TXT */
  QString baseUrl;
  QStringList downloadQueue;
  int updatePeriodSeconds = 600;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_NOAAWEATHERDOWNLOADER_H
