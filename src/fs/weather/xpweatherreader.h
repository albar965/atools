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

#ifndef ATOOLS_XPWEATHERREADER_H
#define ATOOLS_XPWEATHERREADER_H

#include "fs/util/airportcoordtypes.h"
#include "fs/weather/weathertypes.h"

#include <QObject>

namespace atools {
namespace geo {
class Pos;
}
namespace util {
class FileSystemWatcher;
}

namespace fs {
namespace weather {

class Metar;
class MetarIndex;

/*
 * Reads the X-Plane 11 METAR.rwx or X-Plane 12 folder and watches the files/folder for changes.
 */
class XpWeatherReader
  : public QObject
{
  Q_OBJECT

public:
  explicit XpWeatherReader(QObject *parent, bool verboseLogging);
  virtual ~XpWeatherReader() override;

  XpWeatherReader(const XpWeatherReader& other) = delete;
  XpWeatherReader& operator=(const XpWeatherReader& other) = delete;

  /* Get METAR for airport ICAO or empty string if file or airport is not available.
   * Get station and/or nearest METAR */
  const atools::fs::weather::Metar& getXplaneMetar(const QString& station, const atools::geo::Pos& pos);

  /* File is loaded on demand on first call here. X-Plane 11 uses a file and X-Plane 12 a folder. */
  void setWeatherPath(const QString& path, atools::fs::weather::XpWeatherType type);

  /* Remove METARs and stop watching the file */
  void clear();

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  void setFetchAirportCoords(atools::fs::util::AirportCoordFuncType function, void *object);

  /* Print the size of all container classes to detect overflow or memory leak conditions */
  void debugDumpContainerSizes() const;

  /* true if empty */
  bool needsLoading();

  /* load all METAR files */
  void load();

  atools::fs::weather::XpWeatherType getWeatherType() const
  {
    return weatherType;
  }

  const QString& getWeatherPath() const
  {
    return weatherPath;
  }

signals:
  void weatherUpdated();

private:
  void deleteFsWatcher();
  void createFsWatcher();
  bool read(const QStringList& filenames);

  /* Called from fsWatcher */
  void filesUpdated(const QStringList& filenames);
  void dirUpdated(const QString& dir);

  /* Get all METAR files from parent folder */
  QStringList collectWeatherFiles();

  atools::fs::weather::MetarIndex *metarIndex = nullptr;
  atools::util::FileSystemWatcher *fileWatcher = nullptr;

  QString weatherPath; // Folder or file depending on simulator
  QStringList currentMetarFiles; // Set file or collected files from folder

  bool verbose;

  atools::fs::weather::XpWeatherType weatherType = atools::fs::weather::WEATHER_XP_UNKNOWN;

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPWEATHERREADER_H
