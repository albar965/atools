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

#include "grib/gribdownloader.h"

#include "util/httpdownloader.h"

#include "gribreader.h"
#include "exception.h"
#include "atools.h"
#include "zip/gzip.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <cmath>

namespace atools {
namespace grib {

using atools::util::HttpDownloader;

GribDownloader::GribDownloader(QObject *parent, bool logVerbose = false)
  : QObject(parent), verbose(logVerbose)
{
  downloader = new HttpDownloader(parent, verbose);
  downloader->setAcceptEncoding("gzip");
  connect(downloader, &HttpDownloader::downloadFinished, this, &GribDownloader::downloadFinished);
  connect(downloader, &HttpDownloader::downloadFailed, this, &GribDownloader::downloadFailed);
  connect(downloader, &HttpDownloader::downloadSslErrors, this, &GribDownloader::gribDownloadSslErrors);
  connect(downloader, &HttpDownloader::downloadProgress, this, &GribDownloader::gribDownloadProgress);
}

GribDownloader::~GribDownloader()
{
  stopDownload();
  delete downloader;
}

void GribDownloader::startDownload(const QDateTime& timestamp, const QString& baseUrlParam)
{
  baseUrl = baseUrlParam;

  // Stop current request
  if(downloader->isDownloading())
    downloader->cancelDownload();

  retries = 0;

  // Current UTC time
  // Files are updated every 6 hours
  datetime = atools::timeToNextHourInterval(timestamp.isValid() ? timestamp : QDateTime::currentDateTimeUtc(), 6);

  downloader->setUpdatePeriod(UPDATE_PERIOD);
  startDownloadInternal();
}

void GribDownloader::stopDownload()
{
  if(downloader->isDownloading())
    downloader->cancelDownload();

  retries = 0;
  datetime = QDateTime();
  datasets.clear();
}

void GribDownloader::startDownloadInternal()
{
  // https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl?file=gfs.t06z.pgrb2.1p00.anl&lev_200_mb=on&
  // lev_300_mb=on&lev_450_mb=on&lev_700_mb=on&var_UGRD=on&var_VGRD=on&dir=%2Fgfs.2019042606
  // gfs.2019042518
  // gfs.2019042512
  // gfs.2019042506
  // gfs.2019042500

  // Collect surface parameters ======================
  QString levelStr;
  for(int surface : qAsConst(surfaces))
  {
    if(surface > 0)
      levelStr.append(QString("lev_%1_mb=on&").arg(surface));
    else if(surface < 0)
      levelStr.append(QString("lev_%1_m_above_ground=on&").arg(-surface));
    else
      qWarning() << "Invalid surface value" << surface;
  }

  // Collect data parmeters ======================
  QString parameterStr;
  for(const QString& parameter : qAsConst(parameters))
    parameterStr.append(QString("var_%1=on&").arg(parameter));

  // URL ===============================
  // https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl?file=gfs.t00z.pgrb2.1p00.anl&lev_80_m_above_ground=on&
  // lev_150_mb=on&lev_200_mb=on&lev_250_mb=on&lev_300_mb=on&lev_450_mb=on&lev_700_mb=on&var_UGRD=on&var_VGRD=on&dir=%2Fgfs.20190614/00

  // New URL ===============================
  // https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl?file=gfs.t06z.pgrb2.1p00.anl&
  // lev_150_mb=on&lev_200_mb=on&lev_250_mb=on&lev_300_mb=on&lev_450_mb=on&lev_700_mb=on&lev_80_m_above_ground=on&
  // var_UGRD=on&var_VGRD=on&dir=%2Fgfs.20210323%2F06%2Fatmos
  QString base = baseUrl.isEmpty() ? "https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl" : baseUrl;

  // Need to use C locale due to Qt bug which uses system locale to create date
  QLocale cLocale(QLocale::C);
  QString hh(cLocale.toString(datetime, "hh"));
  QString yyyyMMdd(cLocale.toString(datetime, "yyyyMMdd"));

  QString url = base + "?file=gfs.t" + hh + "z.pgrb2.1p00.anl&" + levelStr + parameterStr + "dir=%2Fgfs." +
                yyyyMMdd + "%2F" + hh + "%2Fatmos";

  downloader->setUrl(url);

  qDebug() << Q_FUNC_INFO << "Starting" << url;

  downloader->startDownload();
}

void GribDownloader::downloadFinished(const QByteArray& data, QString downloadUrl)
{
  qDebug() << Q_FUNC_INFO << data.size() << downloadUrl;

  retries = 0;

  try
  {
    // Decode and copy the data
    GribReader reader(verbose);
    reader.readData(atools::zip::gzipDecompressIf(data, Q_FUNC_INFO));
    datasets = reader.getDatasets();
  }
  catch(atools::Exception& e)
  {
    emit gribDownloadFailed(e.what(), 0, downloadUrl);
    return;
  }
  catch(...)
  {
    emit gribDownloadFailed(tr("Unknown error."), 0, downloadUrl);
    return;
  }

  emit gribDownloadFinished(datasets, downloadUrl);
}

void GribDownloader::downloadFailed(const QString& error, int errorCode, QString downloadUrl)
{
  qDebug() << Q_FUNC_INFO << errorCode << error << "retries" << retries;
  if(++retries < MAX_RETRIES)
  {
    // Download failed - try an earlier dataset 6 hours ago
    datetime = datetime.addSecs(-6 * 3600);
    QTimer::singleShot(0, this, &GribDownloader::startDownloadInternal);
  }
  else
  {
    // Failed for good
    retries = 0;
    emit gribDownloadFailed(error, errorCode, downloadUrl);
  }
}

bool atools::grib::GribDownloader::isDownloading() const
{
  return downloader->isDownloading();
}

void GribDownloader::setIgnoreSslErrors(bool value)
{
  downloader->setIgnoreSslErrors(value);
}

void GribDownloader::debugDumpContainerSizes() const
{
  if(downloader != nullptr)
    downloader->debugDumpContainerSizes();
  qDebug() << Q_FUNC_INFO << "surfaces.size()" << surfaces.size();
  qDebug() << Q_FUNC_INFO << "parameters.size()" << parameters.size();
  qDebug() << Q_FUNC_INFO << "datasets.size()" << datasets.size();
}

} // namespace grib
} // namespace atools
