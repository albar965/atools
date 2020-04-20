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

#ifndef ATOOLS_BGL_AP_TRANSITION_H
#define ATOOLS_BGL_AP_TRANSITION_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/approachleg.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum TransitionType
{
  FULL = 1,
  DME = 2
};

namespace tfix {
enum TransitionFixType
{
  VOR = 2,
  NDB = 3,
  TERMINAL_NDB = 4,
  WAYPOINT = 5,
  TERMINAL_WAYPOINT = 6,

  /* From P3D v5 upwards - these are wrong types for this field taken from the XSD.
   * They will be converted to WAYPOINT. */
  // <xs:enumeration value="NDB" />
  // <xs:enumeration value="COURSE_TO_DIST" />
  // <xs:enumeration value="COURSE_TO_ALT" />
  // <xs:enumeration value="HEADING_TO_ALT" />
  // <xs:enumeration value="MANUAL_TERMINATION" />
  // <xs:enumeration value="TERMINAL_NDB" />
  // <xs:enumeration value="TERMINAL_WAYPOINT" />
  // <xs:enumeration value="VOR" />
  // <xs:enumeration value="WAYPOINT" />
  // <xs:enumeration value="RUNWAY" />
  // <xs:enumeration value="LOCALIZER" />

  MANUAL_TERMINATION = 11,
  COURSE_TO_ALT = 12,
  COURSE_TO_DIST = 13,
  HEADING_TO_ALT = 14
};

}

} // namespace ap

/*
 * Transition to an approach. Subrecord of approach.
 */
class Transition :
  public atools::fs::bgl::Record
{
public:
  Transition(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Transition();

  /*
   * @return airport ICAO ident of initial approach fix
   */
  const QString& getFixAirportIdent() const
  {
    return fixAirportIdent;
  }

  /*
   * @return Altitude of the first leg of the transition in meters.
   */
  float getAltitude() const
  {
    return altitude;
  }

  /*
   * @return DME airport ICAO ident if transition is of type DME
   */
  const QString& getDmeAirportIdent() const
  {
    return dmeAirportIdent;
  }

  /*
   * @return DME distance in meter if transition is of type DME
   */
  float getDmeDistance() const
  {
    return dmeDist;
  }

  /*
   * @return DME ICAO ident if transition is of type DME
   */
  const QString& getDmeIdent() const
  {
    return dmeIdent;
  }

  /*
   * @return DME radial in degrees if transition is of type DME
   */
  int getDmeRadial() const
  {
    return dmeRadial;
  }

  /*
   * @return two letter DME ICAO region code if transition is of type DME
   */
  const QString& getDmeRegion() const
  {
    return dmeRegion;
  }

  /*
   * @return initial approach fix two letter ICAO region
   */
  const QString& getFixRegion() const
  {
    return fixRegion;
  }

  /*
   * @return initial approach fix ICAO ident
   */
  const QString& getTransFixIdent() const
  {
    return transFixIdent;
  }

  /*
   * @return type of the initial approach fix
   */
  atools::fs::bgl::ap::tfix::TransitionFixType getTransFixType() const
  {
    return transFixType;
  }

  atools::fs::bgl::ap::TransitionType getType() const
  {
    return type;
  }

  /*
   * @return Get all transition legs
   */
  const QList<atools::fs::bgl::ApproachLeg>& getLegs() const
  {
    return legs;
  }

  static QString transitionTypeToStr(atools::fs::bgl::ap::TransitionType type);
  static QString transitionFixTypeToStr(atools::fs::bgl::ap::tfix::TransitionFixType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Transition& record);

  atools::fs::bgl::ap::TransitionType type;
  atools::fs::bgl::ap::tfix::TransitionFixType transFixType;
  QString transFixIdent, fixRegion, fixAirportIdent;
  float altitude;
  QString dmeIdent, dmeRegion, dmeAirportIdent;
  int dmeRadial;
  float dmeDist;
  QList<atools::fs::bgl::ApproachLeg> legs;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_TRANSITION_H
