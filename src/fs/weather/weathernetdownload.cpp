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

#include "fs/weather/weathernetdownload.h"
#include "util/httpdownloader.h"
#include "fs/weather/weathertypes.h"

namespace atools {
namespace fs {
namespace weather {

WeatherNetDownload::WeatherNetDownload(QObject *parent, int indexSize, bool verboseLogging)
  : QObject(parent), index(indexSize), verbose(verboseLogging)
{
  downloader = new atools::util::HttpDownloader(parent, verboseLogging);

  connect(downloader, &atools::util::HttpDownloader::downloadFinished, this, &WeatherNetDownload::downloadFinished);
  connect(downloader, &atools::util::HttpDownloader::downloadFailed, this, &WeatherNetDownload::downloadFailed);
}

WeatherNetDownload::~WeatherNetDownload()
{
  delete downloader;
}

atools::fs::weather::MetarResult WeatherNetDownload::getMetar(const QString& airportIcao, const atools::geo::Pos& pos)
{
  atools::fs::weather::MetarResult result;
  result.requestIdent = airportIcao;
  result.requestPos = pos;

  if(index.isEmpty())
  {
    if(!downloader->isDownloading())
      downloader->startDownload();
    // else already downloading - message will be sent for update once done
  }
  else
  {
    QString data;
    QString foundKey = index.getTypeOrNearest(data, airportIcao, pos);
    if(!foundKey.isEmpty())
    {
      if(foundKey == airportIcao)
        result.metarForStation = data;
      else
        result.metarForNearest = data;
    }
  }

  result.timestamp = QDateTime::currentDateTime();
  return result;
}

void WeatherNetDownload::setRequestUrl(const QString& url)
{
  downloader->setUrl(url);
}

void WeatherNetDownload::downloadFinished(const QByteArray& data, QString url)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "url" << url << "data size" << data.size();

  parseFile(data);

  emit weatherUpdated();
}

void WeatherNetDownload::downloadFailed(const QString& error, int errorCode, QString url)
{
  qWarning() << Q_FUNC_INFO << "Error downloading from" << url << ":" << error << errorCode;

  emit weatherDownloadFailed(error, errorCode, url);
}

void WeatherNetDownload::setUpdatePeriod(int seconds)
{
  downloader->setUpdatePeriod(seconds);
}

void WeatherNetDownload::updateIndex()
{
  for(auto it = metarMap.begin(); it != metarMap.end(); ++it)
  {
    atools::geo::Pos pos = fetchAirportCoords(it.key());
    if(pos.isValid())
      index.insert(it.key(), it.value(), pos);
  }

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Updated" << index.size() << "metar positions";
}

// AGGH 161200Z 14002KT 9999 FEW016 25/24 Q1010
// AYNZ 160800Z 09005G10KT 9999 SCT030 BKN ABV050 27/24 Q1007 RMK
// AYPY 160700Z 28010KT 9999 SCT025 OVC050 28/23 Q1008 RMK/ BUILD UPS TO S/W
void WeatherNetDownload::parseFile(const QByteArray& data)
{
  QTextStream stream(data, QIODevice::ReadOnly | QIODevice::Text);

  while(!stream.atEnd())
  {
    QString line = stream.readLine().simplified();
    metarMap.insert(line.section(' ', 0, 0), line);
  }

  updateIndex();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Loaded" << data.size() << "bytes and" << metarMap.size()
             << "metars from" << downloader->getUrl();
}

} // namespace weather
} // namespace fs
} // namespace atools
