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

#ifndef BGL_AP_APPROACH_H_
#define BGL_AP_APPROACH_H_

#include "fs/bgl/record.h"
#include "fs/bgl/ap/transition.h"

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum ApproachType
{
  GPS = 0x01,
  VOR = 0x02,
  NDB = 0x03,
  ILS = 0x04,
  LOCALIZER = 0x05,
  SDF = 0x06,
  LDA = 0x07,
  VORDME = 0x08,
  NDBDME = 0x09,
  RNAV = 0x0a,
  LOCALIZER_BACKCOURSE = 0x0b
};

enum ApproachFixType
{
  FIX_VOR = 2,
  FIX_NDB = 3,
  FIX_TERMINAL_NDB = 4,
  FIX_WAYPOINT = 5,
  FIX_TERMINAL_WAYPOINT = 6,
  FIX_RUNWAY = 9
};

} // namespace ap

class Approach :
  public atools::fs::bgl::Record
{
public:
  Approach(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Approach();

  QString getRunwayName() const;

  float getAltitude() const
  {
    return altitude;
  }

  const QString& getFixIdent() const
  {
    return fixIdent;
  }

  const QString& getFixRegion() const
  {
    return fixRegion;
  }

  atools::fs::bgl::ap::ApproachFixType getFixType() const
  {
    return fixType;
  }

  bool isGpsOverlay() const
  {
    return gpsOverlay;
  }

  float getHeading() const
  {
    return heading;
  }

  const QString& getFixAirportIdent() const
  {
    return fixAirportIdent;
  }

  float getMissedAltitude() const
  {
    return missedAltitude;
  }

  int getNumLegs() const
  {
    return numLegs;
  }

  int getNumMissedLegs() const
  {
    return numMissedLegs;
  }

  int getNumTransitions() const
  {
    return numTransitions;
  }

  atools::fs::bgl::ap::ApproachType getType() const
  {
    return type;
  }

  const QList<atools::fs::bgl::Transition>& getTransitions() const
  {
    return transitions;
  }

  bool hasRunwayReference() const
  {
    return runwayNumber > 0;
  }

  static QString approachTypeToStr(atools::fs::bgl::ap::ApproachType type);
  static QString approachFixTypeToStr(atools::fs::bgl::ap::ApproachFixType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Approach& record);

  atools::fs::bgl::ap::ApproachType type;
  int runwayNumber, runwayDesignator;
  bool gpsOverlay;

  int numTransitions, numLegs, numMissedLegs;

  atools::fs::bgl::ap::ApproachFixType fixType;
  QString fixIdent, fixRegion, fixAirportIdent;

  float altitude, heading, missedAltitude;

  QList<atools::fs::bgl::Transition> transitions;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_APPROACH_H_ */
