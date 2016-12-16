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

#ifndef ATOOLS_BGL_NAV_MARKER_H
#define ATOOLS_BGL_NAV_MARKER_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum MarkerType
{
  INNER = 0,
  MIDDLE = 1,
  OUTER = 2,
  BACKCOURSE = 3
};

} // namespace nav

/*
 * Marker beacon
 */
class Marker :
  public atools::fs::bgl::Record
{
public:
  Marker(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Marker();

  const QString& getIdent() const
  {
    return ident;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Two letter ICAO region
   */
  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::MarkerType getType() const
  {
    return type;
  }

  static QString markerTypeToStr(atools::fs::bgl::nav::MarkerType type);

  /*
   * @return Marker heading in degree true
   */
  float getHeading() const
  {
    return heading;
  }

  virtual bool isValid() const override;
  virtual QString getObjectName() const override;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Marker& record);

  atools::fs::bgl::nav::MarkerType type;
  atools::fs::bgl::BglPosition position;
  QString ident, region;
  float heading;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_MARKER_H
