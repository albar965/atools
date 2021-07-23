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

#ifndef ATOOLS_GRIBDOWNLOADER_H
#define ATOOLS_GRIBDOWNLOADER_H

#include "grib/gribcommon.h"

#include <QDateTime>
#include <QObject>
#include <QVector>

namespace atools {

namespace util {
class HttpDownloader;
}
namespace grib {

/*
 * Downloads and decodes GRIB2 files from base URL https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl
 *
 * Example:
 * https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl?file=gfs.t00z.pgrb2.1p00.anl&lev_80_m_above_ground=on&
 * lev_150_mb=on&lev_200_mb=on&lev_250_mb=on&lev_300_mb=on&lev_450_mb=on&lev_700_mb=on&var_UGRD=on&var_VGRD=on&dir=%2Fgfs.20190614/00
 *
 * Only U/V wind components, full earth bounding rectangle and one-degree raster supported.
 *
 */
class GribDownloader :
  public QObject
{
  Q_OBJECT

public:
  GribDownloader(QObject *parent, bool logVerbose);
  ~GribDownloader() override;

  GribDownloader(const GribDownloader& other) = delete;
  GribDownloader& operator=(const GribDownloader& other) = delete;

  /* Start initial download. This will also trigger a recurring download method that gets the file
   * every 30 minutes.
   * Uses the latest file date if timestamp is not set. */
  void startDownload(const QDateTime& timestamp = QDateTime(), const QString& baseUrlParam = QString());

  /* Stop any download in progress */
  void stopDownload();

  /* Positive values are mbar/hpa and negative values as altitude AGL. See web interface for valid values:
   *  https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl */
  const QVector<int>& getSurfaces() const
  {
    return surfaces;
  }

  void setSurfaces(const QVector<int>& value)
  {
    surfaces = value;
  }

  /* Timestamp for downloaded files. */
  const QDateTime& getDatetime() const
  {
    return datetime;
  }

  /* Currently only "UGRD" and "VGRD"
   * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2-0-2.shtml */
  const QStringList& getParameters() const
  {
    return parameters;
  }

  void setParameters(const QStringList& value)
  {
    parameters = value;
  }

  /* Get downloaded and decoded datasets after gribDownloadFinished signal. Valid until next download. */
  const atools::grib::GribDatasetVector& getDatasets() const
  {
    return datasets;
  }

  bool isDownloading() const;

  /* Set to true to ignore any certificate validation or other SSL errors.
   * downloadSslErrors is emitted in case of SSL errors. */
  void setIgnoreSslErrors(bool value);

signals:
  /* Sent if download finished successfully */
  void gribDownloadFinished(const atools::grib::GribDatasetVector& datasets, QString downloadUrl);

  /* Sent if download failed for a reason */
  void gribDownloadFailed(const QString& error, int errorCode, QString downloadUrl);

  /* Emitted on SSL errors. Call setIgnoreSslErrors to ignore future errors and continue.  */
  void gribDownloadSslErrors(const QStringList& errors, const QString& downloadUrl);

  void gribDownloadProgress(qint64 bytesReceived, qint64 bytesTotal, QString downloadUrl);

private:
  void downloadFinished(const QByteArray& data, QString downloadUrl);
  void downloadFailed(const QString& error, int errorCode, QString downloadUrl);
  void startDownloadInternal();

  /* Maximum retries if the data for the current timestamp is not available. Tries current UTC time minus one hour */
  static const int MAX_RETRIES = 4;
  /* Redownload every UPDATE_PERIOD seconds */
  static const int UPDATE_PERIOD = 1800;

  atools::grib::GribDatasetVector datasets;

  atools::util::HttpDownloader *downloader = nullptr;

  QVector<int> surfaces;
  QStringList parameters;
  QDateTime datetime;

  /* Use default if empty */
  QString baseUrl;

  bool verbose = false;
  /* Counter for retries if current date is not available */
  int retries = 0;
};

} // namespace grib
} // namespace atools

#endif // ATOOLS_GRIBDOWNLOADER_H
