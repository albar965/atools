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

#include "fs/common/binarymsageometry.h"

#include "geo/calculations.h"

#include <QDataStream>
#include <QIODevice>

namespace atools {
namespace fs {
namespace common {

using atools::geo::Pos;
using atools::geo::LineString;
using atools::geo::normalizeCourse;

BinaryMsaGeometry::BinaryMsaGeometry(const QByteArray& bytes)
{
  readFromByteArray(bytes);
}

void BinaryMsaGeometry::calculate(const atools::geo::Pos& center, float radiusNm, float magvar, bool trueBearing)
{
  geometry.clear();
  labelPositions.clear();
  bearingEndPositions.clear();

  float radiusMeter = atools::geo::nmToMeter(radiusNm);

  if(trueBearing)
    magvar = 0.f;

  for(int i = 0; i < bearings.size(); i++)
  {
    // +180 to convert bearing relative to navaid to heading
    float bearingFromTrue = normalizeCourse(bearings.at(i) + 180.f + magvar);
    // Roll over to start if last
    float bearingToTrue = normalizeCourse(atools::atRoll(bearings, i + 1) + 180.f + magvar);

    float labelBrg = 0.f; // Default is north of center
    if(bearings.size() > 1)
      // Calculate a bearing for label in the middle of a sector
      labelBrg = normalizeCourse(bearingFromTrue + atools::geo::angleAbsDiff2(bearingFromTrue, bearingToTrue) / 2.f);
    // Calculate label position
    Pos lbl = center.endpoint(radiusMeter / 2.f, labelBrg);

    // Endpoint for bearing line
    Pos from = center.endpoint(radiusMeter, bearingFromTrue);
    LineString geo;
    if(bearings.size() == 1 || (bearings.size() == 2 && atools::almostEqual(bearingFromTrue, bearingToTrue, 0.1f)))
      // Full circle
      geo = LineString(center, radiusMeter, geometryPoints);
    else
      // Arc added to geometry to have endpoints included in circle - helps to avoid not connected or overlapping lines
      geo = LineString(center, from, center.endpoint(radiusMeter, bearingToTrue), true /* clockwise */, geometryPoints);

    geometry.append(geo);
    bearingEndPositions.append(from);
    labelPositions.append(lbl);
  }
  geometry.removeDuplicates();
}

void BinaryMsaGeometry::addSector(float bearingDeg, float altitudeFt)
{
  bearings.append(bearingDeg);
  altitudes.append(altitudeFt);
}

void BinaryMsaGeometry::addSectors(const QVector<float>& bearingDegAltitudeFt)
{
  for(int i = 0; i < bearingDegAltitudeFt.size(); i += 2)
    addSector(bearingDegAltitudeFt.at(i), bearingDegAltitudeFt.at(i + 1));
}

void BinaryMsaGeometry::readFromByteArray(const QByteArray& bytes)
{
  clear();

  QDataStream in(bytes);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  quint16 sizeGeo;
  in >> sizeGeo;
  for(quint16 i = 0; i < sizeGeo; i++)
  {
    float lonx, laty;
    in >> lonx >> laty;
    geometry.append(lonx, laty);
  }

  quint8 size;
  in >> size;
  for(quint8 i = 0; i < size; i++)
  {
    float value;
    in >> value;
    bearings.append(value);

    in >> value;
    altitudes.append(value);

    float lonx, laty;
    in >> lonx >> laty;
    bearingEndPositions.append(lonx, laty);

    in >> lonx >> laty;
    labelPositions.append(lonx, laty);
  }
}

QByteArray BinaryMsaGeometry::writeToByteArray() const
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << static_cast<quint16>(geometry.size());
  for(const atools::geo::Pos& pos : geometry)
    out << pos.getLonX() << pos.getLatY();

  out << static_cast<quint8>(bearings.size());
  for(int i = 0; i < bearings.size(); i++)
  {
    out << bearings.at(i);
    out << altitudes.at(i);
    out << bearingEndPositions.at(i).getLonX() << bearingEndPositions.at(i).getLatY();
    out << labelPositions.at(i).getLonX() << labelPositions.at(i).getLatY();
  }

  return bytes;
}

geo::Rect BinaryMsaGeometry::getBoundingRect() const
{
  return geometry.boundingRect();
}

void BinaryMsaGeometry::clear()
{
  geometry.clear();
  labelPositions.clear();
  bearingEndPositions.clear();
  bearings.clear();
  altitudes.clear();
}

bool BinaryMsaGeometry::isValid() const
{
  return !geometry.isEmpty() && geometry.hasAllValidPoints() && labelPositions.hasAllValidPoints() &&
         !bearings.isEmpty() && !altitudes.isEmpty() &&
         labelPositions.size() == bearings.size() && bearings.size() == altitudes.size();
}

bool BinaryMsaGeometry::isFullCircle() const
{
  return bearings.size() == 1 || (bearings.size() == 2 && atools::almostEqual(bearings.at(0), bearings.at(1), 0.1f));
}

} // namespace common
} // namespace fs
} // namespace atools
