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

#ifndef BGL_NAV_WAYPOINT_H_
#define BGL_NAV_WAYPOINT_H_

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"
#include "fs/bgl/nav/airwayentry.h"

#include <QString>
#include <QList>

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum WaypointType
{
  NAMED = 1,
  UNNAMED = 2,
  VOR = 3,
  NDB = 4,
  OFF_AIRWAY = 5,
  IAF = 6,
  FAF = 7
};

} // namespace nav

class Waypoint :
  public atools::fs::bgl::Record
{
public:
  Waypoint(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Waypoint();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  float getMagVar() const
  {
    return magVar;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::WaypointType getType() const
  {
    return type;
  }

  const QList<atools::fs::bgl::AirwayEntry>& getAirways() const
  {
    return airways;
  }

  static QString waypointTypeToStr(atools::fs::bgl::nav::WaypointType type);

  int getNumVictorAirway() const
  {
    return numVictorAirway;
  }

  int getNumJetAirway() const
  {
    return numJetAirway;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Waypoint& record);

  atools::fs::bgl::nav::WaypointType type;
  atools::fs::bgl::BglPosition position;
  float magVar;
  QString ident, region, airportIdent;
  int numVictorAirway = 0, numJetAirway = 0;
  QList<atools::fs::bgl::AirwayEntry> airways;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_WAYPOINT_H_ */
