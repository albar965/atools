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

class QRegularExpression;

namespace atools {
namespace geo {

/* Simple geographic position. All calculations based on WGS84 ellipsoid */
class Pos
{
public:
  Pos();
  Pos(const Pos& other);

  Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
      int latYDeg, int latYMin, float latYSec, bool south, float alt = 0.f);

  Pos(float longitudeX, float latitudeY, float alt = 0.f);
  Pos(double longitudeX, double latitudeY, float alt = 0.f);

  /* @param str format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  Pos(const QString& str);
  virtual ~Pos();

  Pos& operator=(const Pos& other);

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

  Pos& normalize();
  Pos& toDeg();
  Pos& toRad();

  /* return endpoint at distance and angle */
  Pos endpoint(float distanceMeter, float angle) const;

  /* Distance to other point */
  float distanceMeterTo(const Pos& otherPos) const;

  /* Distance to other point */
  float angleDegTo(const Pos& otherPos) const;

  /* @return format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  QString toLongString() const;

  /* @return format like "49.314,8.543,220" (lonX,latY,alt) */
  QString toString() const;

  bool isValid() const
  {
    return valid;
  }

protected:
  friend QDebug operator<<(QDebug out, const Pos& record);

  // Länge (x),Breite (y)
  float lonX, latY, altitude;

private:
  double calculateAngle(double lonX1, double latY1, double lonX2, double latY2) const;
  double calculateAngle(const Pos& p1, const Pos& p2) const;
  double calculateDistance(double lonX1, double latY1, double lonX2, double latY2) const;
  double calculateDistance(const Pos& p1, const Pos& p2) const;
  Pos calculateEndpoint(double longitude, double latitude, double dist, double angle) const;
  Pos calculateEndpoint(const Pos& p, double dist, double angle) const;

  static const QString LONG_FORMAT, SHORT_FORMAT;
  static const QRegularExpression LONG_FORMAT_REGEXP;
  float sec(float value) const;
  int min(float value) const;
  int deg(float value) const;

  const double PI = 3.14159265358979323846;
  const double EARTH_RADIUS_METER = 6371. * 1000.;
  bool valid = false;
};

} // namespace geo
} // namespace atools

#endif /* ATOOLS_GEO_POSITION_H */
