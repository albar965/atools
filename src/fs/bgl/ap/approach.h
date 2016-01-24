/*
 * Approach.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_AP_APPROACH_H_
#define BGL_AP_APPROACH_H_

#include "../record.h"
#include "../converter.h"
#include "transition.h"

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
  public Record
{
public:
  Approach(atools::io::BinaryStream *bs);
  virtual ~Approach();

  QString getRunwayName() const
  {
    return converter::runwayToStr(runwayNumber, runwayDesignator);
  }

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

  const QList<Transition>& getTransitions() const
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
  friend QDebug operator<<(QDebug out, const Approach& record);

  atools::fs::bgl::ap::ApproachType type;
  int runwayNumber, runwayDesignator;
  bool gpsOverlay;

  int numTransitions, numLegs, numMissedLegs;

  atools::fs::bgl::ap::ApproachFixType fixType;
  QString fixIdent, fixRegion, fixAirportIdent;

  float altitude, heading, missedAltitude;

  QList<Transition> transitions;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_APPROACH_H_ */
