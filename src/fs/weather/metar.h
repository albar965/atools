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

#ifndef  ATOOLS_FS_WEATHER_METAR_H
#define  ATOOLS_FS_WEATHER_METAR_H

#include <QDateTime>
#include <QString>

namespace atools {
namespace fs {
namespace weather {

class MetarParser;

/* Metar parsing utility */
class Metar
{
public:
  Metar();
  Metar(const atools::fs::weather::Metar& other);
  explicit Metar(const QString& metarString, const QString& metarStation = QString(),
                 const QDateTime& metarTimestamp = QDateTime(), bool isSimFormat = false);
  virtual ~Metar();

  Metar& operator=(const atools::fs::weather::Metar& other);

  /* Fixes multiple flaws in FSX metar strings */

  /* Returns a modified version of the Flightgear metar parser. Will never return null. */
  const atools::fs::weather::MetarParser& getParsedMetar() const
  {
    return *parsed;
  }

  const QString& getMetar() const
  {
    return metar;
  }

  const QString& getStation() const
  {
    return station;
  }

  QDateTime getTimestamp() const
  {
    return timestamp;
  }

  const QString& getCleanMetar() const
  {
    return cleanMetar;
  }

  bool isValid() const;

private:
  void buildCleanMetar();

  QString cleanMetar, metar, station;
  bool simFormat = false;
  QDateTime timestamp;
  atools::fs::weather::MetarParser *parsed = nullptr;

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHER_METAR_H
