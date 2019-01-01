/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_NAV_WAYPOINT_H
#define ATOOLS_BGL_NAV_WAYPOINT_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"
#include "fs/bgl/nav/airwaysegment.h"

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

/*
 * Waypoint or intersection. Can be a top level record or a subrecord of airport.
 * Can be a part of an airway and contains a list of all attached airways.
 */
class Waypoint :
  public atools::fs::bgl::Record
{
public:
  Waypoint(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Waypoint();

  /*
   * @return airport ICAO ident if available
   */
  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  /*
   * @return waypoint ICAO ident
   */
  const QString& getIdent() const
  {
    return ident;
  }

  /*
   * @return Magnetic variance for the waypoint. < 0 for West and > 0 for East
   */
  float getMagVar() const
  {
    return magVar;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Two letter ICAO region ident for the navaid if available
   */
  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::WaypointType getType() const
  {
    return type;
  }

  /*
   * @return a list of airway segments. One entry for each airway attached to this waypoint where each entry contains
   * predecessor and successor waypoint.
   */
  const QList<atools::fs::bgl::AirwaySegment>& getAirways() const
  {
    return airways;
  }

  /*
   * @return Number of victor airways attached to this waypoint
   */
  int getNumVictorAirway() const
  {
    return numVictorAirway;
  }

  /*
   * @return Number of jet airways attached to this waypoint
   */
  int getNumJetAirway() const
  {
    return numJetAirway;
  }

  static QString waypointTypeToStr(atools::fs::bgl::nav::WaypointType type);

  virtual bool isValid() const override;
  virtual bool isDisabled() const override;
  virtual QString getObjectName() const override;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Waypoint& record);

  atools::fs::bgl::nav::WaypointType type;
  atools::fs::bgl::BglPosition position;
  float magVar;
  QString ident, region, airportIdent;
  int numVictorAirway = 0, numJetAirway = 0;
  QList<atools::fs::bgl::AirwaySegment> airways;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_WAYPOINT_H
