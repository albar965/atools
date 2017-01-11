/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_AIRPORTAPPROACHLEG_H
#define ATOOLS_BGL_AIRPORTAPPROACHLEG_H

#include "fs/bgl/ap/approachtypes.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

namespace leg {
enum Type
{
  AF = 0x01, /* Arc To a Fix */
  CA = 0x02, /* Course To Altitude */
  CD = 0x03, /* Course To a DME */
  CF = 0x04, /* Course To a Fix */
  CI = 0x05, /* Course To Next Leg Intercept */
  CR = 0x06, /* Course To a Radial */
  DF = 0x07, /* Direct To a Fix */
  FA = 0x08, /* Fix To Altitude */
  FC = 0x09, /* Fix To a Distance on Course */
  FD = 0x0a, /* Fix To a DME Termination */
  FM = 0x0b, /* Fix To a Manual Termination */
  HA = 0x0c, /* Hold to Altitude */
  HF = 0x0d, /* Hold to Fix */
  HM = 0x0e, /* Hold to Manual Termination */
  IF = 0x0f, /* Initial Fix */
  PI = 0x10, /* Procedure Turn */
  RF = 0x11, /* Radius to Fix */
  TF = 0x12, /* Track To a Fix */
  VA = 0x13, /* Heading To Altitude */
  VD = 0x14, /* Heading To DME */
  VI = 0x15, /* Heading To Next Leg Intercept */
  VM = 0x16, /* Heading To Manual Termination */
  VR = 0x17 /* Heading To a Radial */
};

enum AltDescriptor
{
  // TODO not clear - only "A", "+" and "-" appear - BGL compiler documentation is not complete
  UNKNOWN = 0,
  A = 01,
  PLUS = 02, /* Fly at or above altitude1 */
  MINUS = 03, /* Fly at or below altitude1 */
  B = 04
};

enum TurnDirection
{
  NONE = 0,
  L = 1,
  R = 2,
  BOTH = 3
};

}

/*
 * Approach or transition leg. Not an actual record since it does not contain a header or size.
 */
class ApproachLeg
{
public:
  ApproachLeg(atools::io::BinaryStream *bs, bool ismissed);

  atools::fs::bgl::leg::Type getType() const
  {
    return type;
  }

  atools::fs::bgl::leg::AltDescriptor getAltDescriptor() const
  {
    return altDescriptor;
  }

  atools::fs::bgl::leg::TurnDirection getTurnDirection() const
  {
    return turnDirection;
  }

  atools::fs::bgl::ap::fix::ApproachFixType getFixType() const
  {
    return fixType;
  }

  atools::fs::bgl::ap::fix::ApproachFixType getRecommendedFixType() const
  {
    return recommendedFixType;
  }

  /*
   * @return true if is a fly over waypoint otherwise fly by
   */
  bool isFlyover() const
  {
    return flyover;
  }

  QString getFixIdent() const
  {
    return fixIdent;
  }

  QString getFixRegion() const
  {
    return fixRegion;
  }

  QString getFixAirportIdent() const
  {
    return fixAirportIdent;
  }

  QString getRecommendedFixIdent() const
  {
    return recommendedFixIdent;
  }

  QString getRecommendedFixRegion() const
  {
    return recommendedFixRegion;
  }

  /*
   * @return heading in degrees true
   */
  float getTheta() const
  {
    return theta;
  }

  /*
   * @return distance in ? (TODO not clear if meter or NM)
   */
  float getRho() const
  {
    return rho;
  }

  /*
   * @return true if course is degrees true otherwise magnetic
   */
  bool isTrueCourse() const
  {
    return trueCourse;
  }

  /*
   * @return course in degrees true or magnetic depending on isTrueCourse
   */
  float getCourse() const
  {
    return course;
  }

  /*
   * @return true if getDistOrTime gives travelling time in minutes otherwise distance in meter
   */
  bool isTime() const
  {
    return time;
  }

  /*
   * @return gives travelling time in minutes or distance in meter depending on value of getDistOrTime
   */
  float getDistOrTime() const
  {
    return distOrTime;
  }

  /*
   * @return altitude 1 in meter. Meaning depends on AltDescriptor
   */
  float getAltitude1() const
  {
    return altitude1;
  }

  /*
   * @return altitude 2 in meter. Meaning depends on AltDescriptor
   */
  float getAltitude2() const
  {
    return altitude2;
  }

  /*
   * @return true if this is a missed approach leg
   */
  bool isMissed() const
  {
    return missed;
  }

  static QString legTypeToString(atools::fs::bgl::leg::Type type);
  static QString altDescriptorToString(atools::fs::bgl::leg::AltDescriptor altDescr);
  static QString turnDirToString(atools::fs::bgl::leg::TurnDirection turnDir);

private:
  friend QDebug operator<<(QDebug out, const ApproachLeg& record);

  atools::fs::bgl::leg::Type type;
  atools::fs::bgl::leg::AltDescriptor altDescriptor;
  atools::fs::bgl::leg::TurnDirection turnDirection;
  atools::fs::bgl::ap::fix::ApproachFixType fixType, recommendedFixType;

  bool trueCourse, time, flyover, missed;
  QString fixIdent, fixRegion, fixAirportIdent, recommendedFixIdent, recommendedFixRegion;

  float theta /* heading */, rho /* distance */, course, distOrTime, altitude1, altitude2;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTAPPROACHLEG_H
