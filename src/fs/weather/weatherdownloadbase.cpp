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

#include "fs/weather/weatherdownloadbase.h"

#include "util/httpdownloader.h"
#include "fs/weather/weathertypes.h"
#include "fs/weather/metarindex.h"

namespace atools {
namespace fs {
namespace weather {

WeatherDownloadBase::WeatherDownloadBase(QObject *parent, MetarFormat format, bool verboseLogging)
  : verbose(verboseLogging)
{
  metarIndex = new MetarIndex(format, verboseLogging);
  downloader = new atools::util::HttpDownloader(parent, verboseLogging);
  connect(downloader, &atools::util::HttpDownloader::downloadSslErrors,
          this, &WeatherDownloadBase::weatherDownloadSslErrors);
}

WeatherDownloadBase::~WeatherDownloadBase()
{
  delete downloader;
  delete metarIndex;
}

MetarResult WeatherDownloadBase::getMetar(const QString& airportIcao, const geo::Pos& pos)
{
  if(metarIndex->isEmpty())
  {
    if(!isDownloading())
      startDownload();
    // else already downloading - message will be sent for update once done
  }

  // Get real result or empty dummy in case of first call
  return metarIndex->getMetar(airportIcao, pos);
}

void WeatherDownloadBase::setRequestUrl(const QString& url)
{
  downloader->setUrl(url);
}

bool WeatherDownloadBase::isDownloading() const
{
  return downloader->isDownloading();
}

void WeatherDownloadBase::setUpdatePeriod(int seconds)
{
  downloader->setUpdatePeriod(seconds);
}

void WeatherDownloadBase::setFetchAirportCoords(const std::function<geo::Pos(const QString&)>& value)
{
  metarIndex->setFetchAirportCoords(value);
}

int WeatherDownloadBase::size() const
{
  return metarIndex->size();
}

void WeatherDownloadBase::setIgnoreSslErrors(bool value)
{
  downloader->setIgnoreSslErrors(value);
}

void WeatherDownloadBase::startDownload()
{
  if(!downloader->isDownloading())
    downloader->startDownload();
}

} // namespace weather
} // namespace fs
} // namespace atools
