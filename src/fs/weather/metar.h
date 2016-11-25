/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
  Metar(const QString& metarString, const QString& metarStation = QString(),
        const QDateTime& metarTimestamp = QDateTime(), bool isSimFormat = false);
  virtual ~Metar();

  /* Fixes multiple flaws in FSX metar strings */
  QString getCleanMetar() const;

  /* Returns a modified version of the Flightgear metar parser */
  atools::fs::weather::MetarParser getParsedMetar() const;

  static void test();

private:
  QString metar, station;
  bool simFormat;
  const QDateTime timestamp;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHER_METAR_H
