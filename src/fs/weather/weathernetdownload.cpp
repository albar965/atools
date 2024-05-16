/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
#include "fs/weather/metarindex.h"
#include "zip/gzip.h"

namespace atools {
namespace fs {
namespace weather {

WeatherNetDownload::WeatherNetDownload(QObject *parent, atools::fs::weather::MetarFormat format, bool verbose)
  : WeatherDownloadBase(parent, format, verbose)
{
  connect(downloader, &atools::util::HttpDownloader::downloadFinished, this, &WeatherNetDownload::downloadFinished);
  connect(downloader, &atools::util::HttpDownloader::downloadFailed, this, &WeatherNetDownload::downloadFailed);
}

void WeatherNetDownload::downloadFinished(const QByteArray& data, QString url)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "url" << url << "data size" << data.size();

  // Reset error state which avoids triggering downloads
  setErrorStateTimer(false);

  // AGGH 161200Z 14002KT 9999 FEW016 25/24 Q1010
  // AYNZ 160800Z 09005G10KT 9999 SCT030 BKN ABV050 27/24 Q1007 RMK
  // AYPY 160700Z 28010KT 9999 SCT025 OVC050 28/23 Q1008 RMK/ BUILD UPS TO S/W
  QTextStream stream(atools::zip::gzipDecompressIf(data, Q_FUNC_INFO), QIODevice::ReadOnly | QIODevice::Text);
  metarIndex->read(stream, downloader->getUrl(), false /* merge */);

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Loaded" << data.size() << "bytes and" << metarIndex->numStationMetars()
             << "metars from" << downloader->getUrl();

  if(metarIndex->isEmpty())
    emit weatherDownloadFailed(tr("No METARs found in download."), 0, url);
  else
    emit weatherUpdated();
}

void WeatherNetDownload::downloadFailed(const QString& error, int errorCode, QString url)
{
  qWarning() << Q_FUNC_INFO << "Error downloading from" << url << ":" << error << errorCode;

  // Set error state which avoids triggering downloads for a certain period
  setErrorStateTimer();

  emit weatherDownloadFailed(error, errorCode, url);
}

} // namespace weather
} // namespace fs
} // namespace atools
