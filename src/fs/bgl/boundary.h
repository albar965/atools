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

#ifndef ATOOLS_BGL_AIRPORTBOUNDARY_H
#define ATOOLS_BGL_AIRPORTBOUNDARY_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"
#include "fs/bgl/boundarysegment.h"
#include "fs/bgl/ap/com.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace boundary {
enum BoundaryType
{
  NONE = 0x00,
  CENTER = 0x01,
  CLASS_A = 0x02,
  CLASS_B = 0x03,
  CLASS_C = 0x04,
  CLASS_D = 0x05,
  CLASS_E = 0x06,
  CLASS_F = 0x07,
  CLASS_G = 0x08,
  TOWER = 0x09,
  CLEARANCE = 0x0a,
  GROUND = 0x0b,
  DEPARTURE = 0x0c,
  APPROACH = 0x0d,
  MOA = 0x0e,
  RESTRICTED = 0x0f,
  PROHIBITED = 0x10,
  WARNING = 0x11,
  ALERT = 0x12,
  DANGER = 0x13,
  NATIONAL_PARK = 0x14,
  MODEC = 0x15,
  RADAR = 0x16,
  TRAINING = 0x17
};

enum AltitudeType
{
  UNKNOWN = 0,
  MEAN_SEA_LEVEL = 1,
  ABOVE_GROUND_LEVEL = 2,
  UNLIMITED = 3
};

} // namespace boundary

/*
 * Airspace boundary
 */
class Boundary :
  public atools::fs::bgl::Record
{
public:
  Boundary();

  /*
   * Reads an airspace boundary and all subrecords of type name, lines (class BoundaryLine) and
   * optional COM frequency
   */
  Boundary(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Boundary();

  const QString& getName() const
  {
    return name;
  }

  atools::fs::bgl::boundary::BoundaryType getType() const
  {
    return type;
  }

  /*
   *  Bounding rect
   */
  const atools::fs::bgl::BglPosition& getMinPosition() const
  {
    return minPosition;
  }

  /*
   *  Bounding rect
   */
  const atools::fs::bgl::BglPosition& getMaxPosition() const
  {
    return maxPosition;
  }

  atools::fs::bgl::boundary::AltitudeType getMinAltType() const
  {
    return minAltType;
  }

  atools::fs::bgl::boundary::AltitudeType getMaxAltType() const
  {
    return maxAltType;
  }

  const QList<BoundarySegment>& getSegments() const
  {
    return lines;
  }

  bool hasCom() const
  {
    return com;
  }

  atools::fs::bgl::com::ComType getComType() const
  {
    return comType;
  }

  /*
   * @return optional COM frequency in MHz * 1000
   */
  int getComFrequency() const
  {
    return comFrequency;
  }

  const QString& getComName() const
  {
    return comName;
  }

  /* enum to string conversion methods */
  static QString boundaryTypeToStr(atools::fs::bgl::boundary::BoundaryType type);
  static QString altTypeToStr(atools::fs::bgl::boundary::AltitudeType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Boundary& record);

  QString name;
  atools::fs::bgl::boundary::BoundaryType type;
  atools::fs::bgl::BglPosition minPosition, maxPosition;
  atools::fs::bgl::boundary::AltitudeType minAltType, maxAltType;
  QList<BoundarySegment> lines;

  bool com = false;
  atools::fs::bgl::com::ComType comType = atools::fs::bgl::com::NONE;
  int comFrequency = 0;
  QString comName;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTBOUNDARY_H
