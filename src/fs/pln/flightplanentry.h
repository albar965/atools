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

#ifndef FLIGHTPLANENTRY_H
#define FLIGHTPLANENTRY_H

#include <QString>

#include "geo/pos.h"

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace pln {

namespace entry {
enum WaypointType
{
  UNKNOWN,
  AIRPORT,
  INTERSECTION,
  VOR,
  NDB,
  USER
};

}

class FlightplanEntry
{
public:
  FlightplanEntry(atools::sql::SqlDatabase *sqlDb = nullptr);
  ~FlightplanEntry();

  void loadFromWaypoint(int waypointId);
  void loadFromVor(int vorId);
  void loadFromNdb(int ndbId);
  void loadFromCoordinates(const atools::geo::Pos& pos);

  QString getWaypointTypeAsString() const;
  atools::fs::pln::entry::WaypointType getWaypointType() const;
  void setWaypointType(const QString& value);
  void setWaypointType(const atools::fs::pln::entry::WaypointType& value);

  QString getWaypointId() const;
  void setWaypointId(const QString& value);

  QString getAirway() const;
  void setAirway(const QString& value);

  QString getIcaoRegion() const;
  void setIcaoRegion(const QString& value);

  QString getIcaoIdent() const;
  void setIcaoIdent(const QString& value);

  atools::geo::Pos getPosition() const;
  void setPosition(const atools::geo::Pos& value);

private:
  static QString waypointTypeToString(atools::fs::pln::entry::WaypointType type);
  static atools::fs::pln::entry::WaypointType stringToWaypointType(const QString& str);

  atools::fs::pln::entry::WaypointType waypointType;
  QString waypointId, airway, icaoRegion, icaoIdent;
  atools::geo::Pos position;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // FLIGHTPLANENTRY_H
