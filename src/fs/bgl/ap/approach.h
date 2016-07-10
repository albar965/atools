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

#ifndef ATOOLS_BGL_AP_APPROACH_H
#define ATOOLS_BGL_AP_APPROACH_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/transition.h"
#include "fs/bgl/ap/approachleg.h"
#include "fs/bgl/ap/approachtypes.h"

namespace atools {
namespace fs {
namespace bgl {

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

  atools::fs::bgl::ap::fix::ApproachFixType getFixType() const
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

  const QList<atools::fs::bgl::ApproachLeg>& getLegs() const
  {
    return legs;
  }

  const QList<atools::fs::bgl::ApproachLeg>& getMissedLegs() const
  {
    return missedLegs;
  }

  bool hasRunwayReference() const
  {
    return runwayNumber > 0;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Approach& record);

  atools::fs::bgl::ap::ApproachType type;
  int runwayNumber, runwayDesignator;
  bool gpsOverlay;

  int numTransitions;

  atools::fs::bgl::ap::fix::ApproachFixType fixType;
  QString fixIdent, fixRegion, fixAirportIdent;

  float altitude, heading, missedAltitude;

  QList<atools::fs::bgl::Transition> transitions;

  QList<atools::fs::bgl::ApproachLeg> legs, missedLegs;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_APPROACH_H
