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

#include "fs/weather/noaaweatherdownloader.h"

#include "fs/weather/metarindex.h"
#include "util/httpdownloader.h"
#include "geo/pos.h"
#include "fs/weather/weathertypes.h"

namespace atools {
namespace fs {
namespace weather {

using atools::util::HttpDownloader;

NoaaWeatherDownloader::NoaaWeatherDownloader(QObject *parent, int indexSize, bool verboseLogging)
  : QObject(parent), verbose(verboseLogging)
{
  index = new MetarIndex(indexSize, verboseLogging);
  downloader = new HttpDownloader(parent, verbose);

  connect(downloader, &HttpDownloader::downloadFinished, this, &NoaaWeatherDownloader::downloadFinished);
  connect(downloader, &HttpDownloader::downloadFailed, this, &NoaaWeatherDownloader::downloadFailed);

  updateTimer.setSingleShot(true);
  connect(&updateTimer, &QTimer::timeout, this, &NoaaWeatherDownloader::startDownload);
}

NoaaWeatherDownloader::~NoaaWeatherDownloader()
{
  stopTimer();

  delete downloader;
  delete index;
}

void NoaaWeatherDownloader::startTimer()
{
  if(updatePeriodSeconds > 0)
  {
    updateTimer.setInterval(updatePeriodSeconds * 1000);
    updateTimer.start();
  }
  else
    updateTimer.stop();
}

void NoaaWeatherDownloader::stopTimer()
{
  updateTimer.stop();
}

MetarResult NoaaWeatherDownloader::getMetar(const QString& airportIcao, const geo::Pos& pos)
{
  return index->getMetar(airportIcao, pos);
}

void NoaaWeatherDownloader::setRequestUrl(const QString& url)
{
  baseUrl = url;
}

void NoaaWeatherDownloader::setFetchAirportCoords(const std::function<geo::Pos(const QString&)>& value)
{
  index->setFetchAirportCoords(value);
}

void NoaaWeatherDownloader::setUpdatePeriod(int seconds)
{
  updatePeriodSeconds = seconds;
}

bool NoaaWeatherDownloader::isDownloading() const
{
  return downloader->isDownloading() || !downloadQueue.isEmpty();
}

void NoaaWeatherDownloader::updateIndex()
{
  index->updateIndex();
}

bool NoaaWeatherDownloader::read(const QByteArray& data, const QString& url)
{
  QTextStream stream(data);
  return index->read(stream, url, true /* merge */);
}

void NoaaWeatherDownloader::downloadFinished(const QByteArray& data, QString url)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "downloadQueue" << downloadQueue;

  if(read(data, url) && downloadQueue.isEmpty())
    // Notification only if no outstanding downloads
    emit weatherUpdated();

  if(downloadQueue.isEmpty())
    startTimer();

  // Start later in the event queue to allow the download to finish
  QTimer::singleShot(0, this, &NoaaWeatherDownloader::download);
}

void NoaaWeatherDownloader::downloadFailed(const QString& error, int errorCode, QString url)
{
  emit weatherDownloadFailed(error, errorCode, url);

  startTimer();

  // Start later in the event queue to allow the download to finish
  QTimer::singleShot(0, this, &NoaaWeatherDownloader::download);
}

void NoaaWeatherDownloader::startDownload()
{
  // Start only if not active and if download queue is empty
  if(!isDownloading())
  {
    // Current UTC time
    // Files are updated every hour
    QDateTime datetime = QDateTime::currentDateTimeUtc();
    datetime.setTime(QTime(datetime.time().hour(), 0, 0));

    int startOffset = 1;
    appendJob(datetime, startOffset--); // UTC + 1 - either old or newly populated
    appendJob(datetime, startOffset--); // UTC - current
    appendJob(datetime, startOffset--); // UTC - 1 hour - older which might be still populated

    download();
  }
}

void NoaaWeatherDownloader::appendJob(QDateTime datetime, int timeOffsetHour)
{
  datetime = datetime.addSecs(timeOffsetHour * 3600);
  QString url = baseUrl.arg(datetime.time().hour(), 2, 10, QChar('0'));

  // Do not append duplicates
  if(!downloadQueue.contains(url))
    downloadQueue.append(url);
}

void NoaaWeatherDownloader::download()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "downloadQueue" << downloadQueue;

  if(!downloader->isDownloading())
  {
    if(!downloadQueue.isEmpty())
    {
      downloader->setUrl(downloadQueue.takeFirst());
      downloader->startDownload();
    }
  }
}

} // namespace weather
} // namespace fs
} // namespace atools
