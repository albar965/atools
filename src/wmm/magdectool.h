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

#ifndef MAGDECTOOL_H
#define MAGDECTOOL_H

#include "atools.h"

#include <QApplication>
#include <QDate>

namespace atools {
namespace geo {
class Pos;
}
namespace wmm {

/*
 * Interface to GeomagnetismLibrary. Calculates an array for 360 x 181 values and provides accessors and
 * interpolation methods to this array (i.e. one degree grid).
 *
 * Declination values are not calculated on the fly.
 */
class MagDecTool
{
  Q_DECLARE_TR_FUNCTIONS(MagDecTool)

public:
  MagDecTool();
  ~MagDecTool();

  MagDecTool(const MagDecTool& other) = delete;
  MagDecTool& operator=(const MagDecTool& other) = delete;

  /* Build the declination array for current year/month or given values. January = 1 */
  void init(int year = 0, int month = 1);
  void init(const QDate& dateTime);

  /* Get version information for the GeomagnetismLibrary */
  QString getVersion() const;

  /* Get magnetic variance/declination. Positive is east and negative is west. */
  float getMagVar(const atools::geo::Pos& pos);

  float getMagVar(int col, int row)
  {
    return magdecGrid[col + 180 + (row + 90) * 360];
  }

  /* Reference date as given or current date */
  const QDate& getReferenceDate() const
  {
    return referenceDate;
  }

  /* Valid if data is calculated */
  bool isValid() const
  {
    return magdecGrid != nullptr;
  }

  /* Frees all and sets status to invalid */
  void clear();

#ifdef WRITE_GEOID_BUFFER
  void writeGeoidBuffer();

#endif

private:
  float getMagVar(float col, float row)
  {
    return magdecGrid[atools::roundToInt(col) + 180 + (atools::roundToInt(row) + 90) * 360];
  }

  // Read EGM9615.buf - only needed for grid creating
  QVector<float> readGeoidBuffer();

  // latY (-90 to 90), lonX (-180 to 179)
  // -90.00 -180.00, -90.00 -179.00 ... 90.00 178.00, 90.00 179.00
  float *magdecGrid = nullptr;

  QDate referenceDate;
};

} // namespace wmm
} // namespace atools

#endif // MAGDECTOOL_H
