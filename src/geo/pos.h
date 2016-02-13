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

#ifndef ATOOLS_GEO_POSITION_H
#define ATOOLS_GEO_POSITION_H

#include "logging/loggingdefs.h"

namespace atools {
namespace geo {

/* Simple geographic position */
class Pos
{
public:
  Pos();
  Pos(int lonXDeg,
      int lonXMin,
      float lonXSec,
      bool west,
      int latYDeg,
      int latYMin,
      float latYSec,
      bool south,
      float alt = 0.f);
  Pos(float longitudeX, float latitudeY, float alt = 0.f);
  /* @param str format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  Pos(const QString& str);
  ~Pos();

  float getLatY() const
  {
    return latY;
  }

  int getLatYDeg() const;
  int getLatYMin() const;
  float getLatYSec() const;

  float getLonX() const
  {
    return lonX;
  }

  int getLonXDeg() const;
  int getLonXMin() const;
  float getLonXSec() const;

  float getAltitude() const
  {
    return altitude;
  }

  /* @return format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  QString toLongString() const;

protected:
  friend QDebug operator<<(QDebug out, const Pos& record);

  // Länge (x),Breite (y)
  float lonX, latY, altitude;

private:
  static const QString LONG_FORMAT;
  static const QRegularExpression LONG_FORMAT_REGEXP;
  float sec(float value) const;
  int min(float value) const;
  int deg(float value) const;

};

} // namespace geo
} // namespace atools

#endif /* ATOOLS_GEO_POSITION_H */
