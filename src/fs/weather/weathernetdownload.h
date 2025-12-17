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

#ifndef ATOOLS_FS_WEATHERNETDOWNLOAD_H
#define ATOOLS_FS_WEATHERNETDOWNLOAD_H

#include "fs/weather/weatherdownloadbase.h"

namespace atools {
namespace util {
class HttpDownloader;
}
namespace fs {
namespace weather {

class MetarIndex;

/*
 * Manages metar files that are download fully from the web like IVAO.
 * Has a timer that triggers a recurrent lookup.
 */
class WeatherNetDownload :
  public WeatherDownloadBase
{
  Q_OBJECT

public:
  explicit WeatherNetDownload(QObject *parent, atools::fs::weather::MetarFormat format, bool verbose);
  virtual ~WeatherNetDownload() override;

private:
  void downloadFinished(const QByteArray& data, QString url);
  void downloadFailed(const QString& error, int errorCode, QString url);

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHERNETDOWNLOAD_H
