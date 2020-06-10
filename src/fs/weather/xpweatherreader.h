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

#ifndef ATOOLS_XPWEATHERREADER_H
#define ATOOLS_XPWEATHERREADER_H

#include <QObject>
#include <functional>

namespace atools {
namespace geo {
class Pos;
}
namespace util {
class FileSystemWatcher;
}

namespace fs {
namespace weather {

struct MetarResult;
class MetarIndex;

/*
 * Reads the X-Plane METAR.rwx the watches the file for changes.
 */
class XpWeatherReader
  : public QObject
{
  Q_OBJECT

public:
  explicit XpWeatherReader(QObject *parent, bool verboseLogging);
  virtual ~XpWeatherReader() override;

  /* Get METAR for airport ICAO or empty string if file or airport is not available */

  /* Get station and/or nearest METAR */
  atools::fs::weather::MetarResult getXplaneMetar(const QString& station, const atools::geo::Pos& pos);

  /* File is loaded on demand on first access */
  void setWeatherFile(const QString& file);

  /* Remove METARs and stop watching the file */
  void clear();

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value);

signals:
  void weatherUpdated();

private:
  /* Read METAR.rwx and watch the file if needed */
  void readWeatherFile();

  void deleteFsWatcher();
  void createFsWatcher();
  void pathChanged(const QString& filename);
  bool read();

  atools::fs::weather::MetarIndex *metarIndex = nullptr;
  atools::util::FileSystemWatcher *fsWatcher = nullptr;
  QString weatherFile;

  bool verbose;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPWEATHERREADER_H
