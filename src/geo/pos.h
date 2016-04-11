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

/* Simple geographic position. Calculations based on
 *  http://williams.best.vwh.net/avform.htm */
class Pos final
{
public:
  Pos();
  Pos(const Pos &other);

  Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
      int latYDeg, int latYMin, float latYSec, bool south, float alt = 0.f);

  Pos(float longitudeX, float latitudeY, float alt = 0.f);
  Pos(double longitudeX, double latitudeY, float alt = 0.f);

  /* @param str format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  Pos(const QString &str);

  Pos& operator=(const Pos& other);

  /* Does not compare altitude */
  bool operator==(const Pos& other) const;

  /* Does not compare altitude */
  bool operator!=(const Pos& other) const
  {
    return !(*this == other);
  }

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
  Pos endpoint(float distanceMeter, float angleDeg) const;

  /* Distance to other point in simple units */
  float distanceSimpleTo(const Pos& otherPos) const;

  /* Distance to other point for great circle route */
  float distanceMeterTo(const Pos& otherPos) const;

  /* Distance to line formed by pos1 and pos2. Positive means right of course,
   * negative means left of course. valid if perpendicular point can be found on route. */
  float distanceMeterToLine(const Pos& pos1, const Pos& pos2, bool& validPos) const;

  /* Distance to other point (initial course) */
  float angleDegTo(const Pos& otherPos) const;

  /* return endpoint at distance and angle for a rhumb line (constant course) */
  Pos endpointRhumb(float distanceMeter, float angleDeg) const;

  /* Distance to other point for rhumb line */
  float distanceMeterToRhumb(const Pos& otherPos) const;

  /* Distance to other point using a rhumb line */
  float angleDegToRhumb(const Pos& otherPos) const;

  /* @return format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  QString toLongString() const;

  /* @return format like "49.314,8.543,220" (lonX,latY,alt) */
  QString toString() const;

  bool isValid() const;

  /* Return true if close to any pole */
  bool isPole() const;

  /* Find point between start and end on GC route */
  Pos interpolate(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const;
  Pos interpolate(const atools::geo::Pos& otherPos, float fraction) const;
  void interpolatePoints(const atools::geo::Pos& otherPos, float distanceMeter, int numPoints,
                         QList<Pos>& positions) const;

  /* Find point between start and end on rhumb line */
  Pos interpolateRhumb(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const;
  Pos interpolateRhumb(const atools::geo::Pos& otherPos, float fraction) const;

protected:
  const float INVALID_ORD = std::numeric_limits<float>::max();
  const float EPSILON = std::numeric_limits<float>::epsilon();

  // Länge (x),Breite (y)
  float lonX, latY, altitude;

private:
  friend QDebug operator<<(QDebug out, const atools::geo::Pos& record);

  float sec(float value) const;
  int min(float value) const;
  int deg(float value) const;

  double distanceRad(double lonX1, double latY1, double lonX2, double latY2) const;
  double courseRad(double lonX1, double latY1, double lonX2, double latY2) const;

};

const atools::geo::Pos EMPTY_POS;

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Pos, Q_PRIMITIVE_TYPE);

#endif /* ATOOLS_GEO_POSITION_H */
