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

#ifndef ATOOLS_BINARYMSAGEOMETRY_H
#define ATOOLS_BINARYMSAGEOMETRY_H

#include "geo/linestring.h"

class QByteArray;

namespace atools {
namespace fs {
namespace common {

/*
 * Calculates airport MSA bearings and circles for easier and faster map display.
 * Also provides methods to serialize data into and from binary format.
 */
class BinaryMsaGeometry
{
public:
  BinaryMsaGeometry()
  {

  }

  BinaryMsaGeometry(const QByteArray& bytes);

  /* Bearing is true or mag.
   * For a MSA that is a circle, no sectors both the
   * start and the end bearing set to 180.
   * The bearings are entered clockwise.
   * Call these before running calculate() */
  void addSector(float bearingDeg, float altitudeFt);

  /* Add sectors from a vector with bearing/altitude pairs */
  void addSectors(const QVector<float>& bearingDegAltitudeFt);

  /* Calculate circle/arc geomentry, label points and bearing endpoints */
  void calculate(const atools::geo::Pos& center, float radiusNm, float magvar, bool trueBearing);

  /* Read from database BLOB */
  void readFromByteArray(const QByteArray& bytes);

  /* Write to databasee BLOB after calculation */
  QByteArray writeToByteArray() const;

  /* Bounding rectangle of circle/arcs */
  atools::geo::Rect getBoundingRect() const;

  /* Clear all calculation results and bearing/altitude values */
  void clear();

  bool isValid() const;

  /* Only one bearing and one altitude */
  bool isFullCircle() const;

  /* Bearings in true degree - same size as altitudes */
  const QVector<float>& getBearings() const
  {
    return bearings;
  }

  /* Altitudes in feet - same size as bearings */
  const QVector<float>& getAltitudes() const
  {
    return altitudes;
  }

  /* Outer circle/arcs geometry. 180 points for full circle. */
  const atools::geo::LineString& getGeometry() const
  {
    return geometry;
  }

  /* Pre-calculated altitude label positions. */
  const atools::geo::LineString& getLabelPositions() const
  {
    return labelPositions;
  }

  /* Endpoints for bearing lines from center to end point */
  const atools::geo::LineString& getBearingEndPositions() const
  {
    return bearingEndPositions;
  }

  /* Accuracy for outer circle geometry. Number of positions for full circle */
  void setGeometryPoints(int value)
  {
    geometryPoints = value;
  }

private:
  int geometryPoints = 90;
  QVector<float> bearings, altitudes;
  atools::geo::LineString geometry, labelPositions, bearingEndPositions;
};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_BINARYMSAGEOMETRY_H
