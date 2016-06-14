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

#ifndef ATOOLS_BGL_NAV_AIRWAYENTRY_H
#define ATOOLS_BGL_NAV_AIRWAYENTRY_H

#include "fs/bgl/bglbase.h"
#include "fs/bgl/nav/airwaywaypoint.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {
enum AirwayType
{
  NONE = 0,
  VICTOR = 1,
  JET = 2,
  BOTH = 3
};

} // namespace nav

class AirwayEntry :
  public atools::fs::bgl::BglBase
{
public:
  AirwayEntry(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~AirwayEntry();

  bool hasNextWaypoint() const;

  bool hasPreviousWaypoint() const;

  const atools::fs::bgl::AirwayWaypoint& getNextWaypoint() const
  {
    return next;
  }

  const atools::fs::bgl::AirwayWaypoint& getPreviousWaypoint() const
  {
    return previous;
  }

  const QString& getName() const
  {
    return name;
  }

  atools::fs::bgl::nav::AirwayType getType() const
  {
    return type;
  }

  static QString airwayTypeToStr(atools::fs::bgl::nav::AirwayType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::AirwayEntry& record);

  atools::fs::bgl::nav::AirwayType type;
  QString name;

  atools::fs::bgl::AirwayWaypoint next;
  atools::fs::bgl::AirwayWaypoint previous;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_AIRWAYENTRY_H
