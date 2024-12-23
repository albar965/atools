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

#ifndef ATOOLS_BGL_AIRPORTTAXIPATH_H
#define ATOOLS_BGL_AIRPORTTAXIPATH_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/ap/rw/runway.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace taxipath {
enum Type
{
  UNKNOWN = 0,
  TAXI = 1,
  RUNWAY = 2,
  PARKING = 3,
  PATH = 4,
  CLOSED = 5,
  VEHICLE = 6,
  ROAD = 7, /* MSFS 2024 */
  PAINTEDLINE = 8 /* MSFS 2024 */
};

enum EdgeType
{
  NONE = 0,
  SOLID = 1,
  DASHED = 2,
  SOLID_DASHED = 3
};

}

/*
 *  Taxiway path is a segment of the taxiway network and a subrecord of airport.
 *  Includes runway and vehicle paths.
 */
class TaxiPath
{
public:
  TaxiPath(atools::io::BinaryStream *stream, atools::fs::bgl::StructureType structureType);

  /*
   * @return Taxiway name or full runway name if Type is RUNWAY
   */
  QString getName() const;

  const atools::fs::bgl::TaxiPoint& getStartPoint() const
  {
    return startPos;
  }

  const atools::fs::bgl::TaxiPoint& getEndPoint() const
  {
    return endPos;
  }

  /*
   * @return if true draw the underlying surface
   */
  bool isDrawSurface() const
  {
    return drawSurface;
  }

  /*
   * @return if true draw the detail texture surface
   */
  bool isDrawDetail() const
  {
    return drawDetail;
  }

  bool hasCenterline() const
  {
    return centerline;
  }

  bool hasCenterlineLight() const
  {
    return centerlineLight;
  }

  bool hasLeftEdgeLight() const
  {
    return leftEdgeLight;
  }

  bool hasRightEdgeLight() const
  {
    return rightEdgeLight;
  }

  atools::fs::bgl::Surface getSurface() const
  {
    return surface;
  }

  atools::fs::bgl::taxipath::Type getType() const
  {
    return type;
  }

  /*
   * @return width in meter
   */
  float getWidth() const
  {
    return width;
  }

  const QUuid& getMaterialUuid() const
  {
    return materialUuid;
  }

  static QString pathTypeToString(taxipath::Type type);
  static QString edgeTypeToString(taxipath::EdgeType type);

  bool isValid() const
  {
    return startPos.isValid() && endPos.isValid();
  }

private:
  friend class Airport;
  friend QDebug operator<<(QDebug out, const TaxiPath& record);

  QString taxiName;
  int startIndex, endIndex, runwayDesignator, nameIndex;

  atools::fs::bgl::taxipath::Type type;
  atools::fs::bgl::taxipath::EdgeType leftEdge;
  atools::fs::bgl::taxipath::EdgeType rightEdge;

  atools::fs::bgl::Surface surface;
  float width;

  atools::fs::bgl::TaxiPoint startPos, endPos;

  bool drawSurface, drawDetail, centerline, centerlineLight, leftEdgeLight, rightEdgeLight;
  QUuid materialUuid;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTTAXIPATH_H
