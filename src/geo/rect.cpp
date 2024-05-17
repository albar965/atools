/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "geo/rect.h"
#include "geo/calculations.h"
#include "atools.h"
#include "geo/linestring.h"

#include <QDataStream>

namespace atools {
namespace geo {

Rect::Rect()
{
}

Rect::Rect(const Rect& other)
{
  *this = other;
}

Rect::Rect(const Pos& singlePos)
{
  topLeft = singlePos;
  bottomRight = singlePos;
}

Rect::Rect(const Pos& topLeftPos, const Pos& bottomRightPos)
{
  topLeft = topLeftPos;
  bottomRight = bottomRightPos;
}

Rect::Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY)
{
  topLeft = Pos(leftLonX, topLatY);
  bottomRight = Pos(rightLonX, bottomLatY);
}

Rect::Rect(double leftLonX, double topLatY, double rightLonX, double bottomLatY)
{
  topLeft = Pos(leftLonX, topLatY);
  bottomRight = Pos(rightLonX, bottomLatY);
}

Rect::Rect(const LineString& linestring)
{
  *this = extend(linestring);
}

Rect::Rect(float lonX, float latY)
{
  topLeft = Pos(lonX, latY);
  bottomRight = Pos(lonX, latY);
}

Rect::Rect(double lonX, double latY)
{
  topLeft = Pos(lonX, latY);
  bottomRight = Pos(lonX, latY);
}

Rect::Rect(const Pos& center, float radiusMeter, bool fast)
{
  float east, west;
  if(fast)
  {
    // Do approximation avoiding complex calculations
    float meterForDeg = center.meterForDegreeLonx();
    east = center.getLonX() + radiusMeter / meterForDeg;
    west = center.getLonX() - radiusMeter / meterForDeg;
  }
  else
  {
    // Calculate exact left and right boundaries
    east = center.endpoint(radiusMeter, 90.).getLonX();
    west = center.endpoint(radiusMeter, 270.).getLonX();
  }

  float radiusNm = meterToNm(radiusMeter);
  float north = center.getLatY() + radiusNm / 60.f;
  float south = center.getLatY() - radiusNm / 60.f;

  if(north > 90.f || south < -90.f)
  {
    east = 180.f;
    west = -180.f;
  }

  if(north > 90.f)
    north = 90.f;
  if(south < -90.f)
    south = -90.f;

  topLeft = Pos(west, north);
  bottomRight = Pos(east, south);
}

bool Rect::operator==(const Rect& other) const
{
  return topLeft == other.topLeft && bottomRight == other.bottomRight;
}

bool Rect::almostEqual(const Rect& other, float epsilon) const
{
  return topLeft.almostEqual(other.topLeft, epsilon) && bottomRight.almostEqual(other.bottomRight, epsilon);
}

bool Rect::contains(const Pos& pos) const
{
  if(isValid() || pos.isValid())
  {
    for(const Rect& r : splitAtAntiMeridian())
    {
      if(r.getWest() <= pos.getLonX() && pos.getLonX() <= r.getEast() &&
         r.getNorth() >= pos.getLatY() && pos.getLatY() >= r.getSouth())
        return true;
    }
  }
  return false;
}

bool Rect::overlaps(const Rect& other) const
{

  if(!isValid() || !other.isValid())
    return false;

  if(isPoint() && other.isPoint())
    return *this == other;

  float north = getNorth(), south = getSouth(), east = getEast(), west = getWest();
  float north2 = other.getNorth(), south2 = other.getSouth(), east2 = other.getEast(), west2 = other.getWest();

  // check the intersection criterion for the latitude first
  if((north >= north2 && south <= north2) || // Case 1: northern boundary of other box intersects
     (north2 >= north && south2 <= north) || // Case 2: northern boundary of this box intersects
     (north >= south2 && south <= south2) || // Case 3: southern boundary of other box intersects
     (north2 >= south && south2 <= south)) // Case 4: southern boundary of this box intersects
  {
    if(!crossesAntiMeridian())
    {
      if(!other.crossesAntiMeridian())
      {
        // "Normal" case: both bounding boxes don't cross the date line
        if((east >= east2 && west <= east2) || // Case 1: eastern boundary of other box intersects
           (east2 >= east && west2 <= east) || // Case 2: eastern boundary of this box intersects
           (east >= west2 && west <= west2) || // Case 3: western boundary of other box intersects
           (east2 >= west && west2 <= west)) // Case 4: western boundary of this box intersects
          return true;
      }
      else
      {
        // The other bounding box crosses the date line, "this" one does not:
        // So the date line splits the other bounding box in two parts.
        if(west <= east2 || east >= west2)
          return true;
      }
    }
    else
    {
      if(other.crossesAntiMeridian())
        // The trivial case: both bounding boxes cross the date line and intersect
        return true;
      else
      {
        // "This" bounding box crosses the date line, the other one does not.
        // So the date line splits "this" bounding box in two parts.
        //
        // This also covers the case where this bounding box covers the whole
        // longitude range ( -180 <= lon <= + 180 ).
        if(west2 <= east || east2 >= west)
          return true;
      }
    }
  }

  return false;
}

atools::geo::Rect& Rect::inflateMeter(float width, float height)
{
  return inflate(meterToNm(getWidthMeter() / 60.f) / getWidthDegree() * meterToNm(width) / 60.f, meterToNm(height) / 60.f);
}

Rect& Rect::inflate(float degreesLon, float degreesLat)
{
  if(!isValid())
    return *this;

  if(getWest() - degreesLon > -180.f)
    topLeft.setLonX(getWest() - degreesLon);
  else
    topLeft.setLonX(-180.f);

  if(getEast() + degreesLon < 180.f)
    bottomRight.setLonX(getEast() + degreesLon);
  else
    bottomRight.setLonX(180.f);

  if(getNorth() + degreesLat < 90.f)
    topLeft.setLatY(getNorth() + degreesLat);
  else
    topLeft.setLatY(90.f);

  if(getSouth() - degreesLat > -90.f)
    bottomRight.setLatY(getSouth() - degreesLat);
  else
    bottomRight.setLatY(-90.f);
  return *this;
}

bool Rect::isPoint(float epsilonDegree) const
{
  return isValid() &&
         atools::almostEqual(topLeft.getLonX(), bottomRight.getLonX(), epsilonDegree) &&
         atools::almostEqual(topLeft.getLatY(), bottomRight.getLatY(), epsilonDegree);
}

Rect& Rect::toDeg()
{
  topLeft.toDeg();
  bottomRight.toDeg();
  return *this;
}

Rect& Rect::toRad()
{
  topLeft.toRad();
  bottomRight.toRad();
  return *this;
}

float Rect::getWidthDegree() const
{
  if(crossesAntiMeridian())
    return 180.f - topLeft.getLonX() + bottomRight.getLonX() + 180.f;
  else
    return bottomRight.getLonX() - topLeft.getLonX();
}

float Rect::getHeightDegree() const
{
  return topLeft.getLatY() - bottomRight.getLatY();
}

float Rect::getWidthMeter() const
{
  float centerY = topLeft.getLatY() - getHeightDegree() / 2.f;
  return Pos(topLeft.getLonX(), centerY).distanceMeterTo(Pos(bottomRight.getLonX(), centerY));
}

float Rect::getHeightMeter() const
{
  float centerX = bottomRight.getLonX() - getWidthDegree() / 2.f;
  return Pos(centerX, topLeft.getLatY()).distanceMeterTo(Pos(centerX, bottomRight.getLatY()));
}

float Rect::getDiameterMeter() const
{
  return (getTopLeft().distanceMeterTo(getBottomRight()) + getTopRight().distanceMeterTo(getBottomLeft())) / 2.f;
}

Rect& Rect::extend(const Pos& pos)
{
  if(!pos.isValid())
    return *this;

  if(isValid())
    bounding(*this, LineString({pos, getTopLeft(), getTopRight(), getBottomRight(), getBottomLeft()}));
  else
    // Create single point rect if this is not valid
    *this = Rect(pos);
  return *this;
}

Rect& Rect::extend(const Rect& rect)
{
  if(!rect.isValid())
    return *this;

  if(isValid())
    bounding(*this, LineString({rect.getTopLeft(), rect.getTopRight(), rect.getBottomRight(), rect.getBottomLeft(),
                                getTopLeft(), getTopRight(), getBottomRight(), getBottomLeft()}));
  else
    // Use other if this is not valid
    *this = rect;
  return *this;
}

Rect& Rect::extend(const atools::geo::LineString& linestring)
{
  extend(linestring.boundingRect());
  return *this;
}

Rect& Rect::scale(float horizontalFactor, float verticalFactor)
{
  float deltaY = 0.5f * getHeightDegree() * verticalFactor;
  float deltaX = 0.5f * getWidthDegree() * horizontalFactor;

  Pos center = getCenter();
  float north = std::min((center.getLatY() + deltaY), 90.f);
  float south = std::max((center.getLatY() - deltaY), -90.f);
  float east, west;
  if(deltaX > 180.f)
  {
    east = 180.f;
    west = -180.f;
  }
  else
  {
    east = normalizeLonXDeg(center.getLonX() + deltaX);
    west = normalizeLonXDeg(center.getLonX() - deltaX);
  }
  topLeft.setLonX(west);
  topLeft.setLatY(north);
  bottomRight.setLonX(east);
  bottomRight.setLatY(south);
  return *this;
}

Pos Rect::getCenter() const
{
  if(isValid())
  {
    if(crossesAntiMeridian())
      return Pos(bottomRight.getLonX() + 360.f - (bottomRight.getLonX() + 360.f - topLeft.getLonX()) / 2.f,
                 (topLeft.getLatY() + bottomRight.getLatY()) / 2.f).normalize();
    else
      return Pos((topLeft.getLonX() + bottomRight.getLonX()) / 2.f, (topLeft.getLatY() + bottomRight.getLatY()) / 2.f);
  }
  else
    return EMPTY_POS;
}

bool Rect::crossesAntiMeridian() const
{
  // This calculation requires correct east and west values
  return getEast() < getWest() ||
         (atools::almostEqual(getWest(), 180.f) && atools::almostEqual(getEast(), -180.f)) ||
         (atools::almostEqual(getEast(), 180.f) && atools::almostEqual(getWest(), -180.f));
}

const QList<Rect> Rect::splitAtAntiMeridian() const
{
  if(isValid())
  {
    if(crossesAntiMeridian())
      return QList<Rect>({Rect(getWest(), getNorth(), 180.f, getSouth()), Rect(-180.f, getNorth(), getEast(), getSouth())});
    else
      return QList<Rect>({*this});
  }
  else
    return QList<Rect>();
}

void Rect::swap(Rect& other)
{
  topLeft.swap(other.topLeft);
  bottomRight.swap(other.bottomRight);
}

QString Rect::toString() const
{
  return topLeft.toString() + "," + bottomRight.toString();
}

Rect& Rect::normalize()
{
  topLeft.normalize();
  bottomRight.normalize();
  return *this;
}

Rect Rect::normalized() const
{
  Rect retval(*this);
  return retval.normalize();
}

QDebug operator<<(QDebug out, const Rect& rect)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Rect(" << rect.toString() << ")";
  return out;
}

QDataStream& operator<<(QDataStream& out, const Rect& obj)
{
  bool validDummy = obj.isValid();
  out << obj.topLeft << obj.bottomRight << validDummy;
  return out;
}

QDataStream& operator>>(QDataStream& in, Rect& obj)
{
  bool validDummy;
  in >> obj.topLeft >> obj.bottomRight >> validDummy;
  return in;
}

} // namespace geo
} // namespace atools
