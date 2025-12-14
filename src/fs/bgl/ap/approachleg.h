/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
#include "fs/bgl/recordtypes.h"

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
  // Definition/Description: The "Altitude Description” field
  // will designate whether a waypoint should be crossed "at,”
  // "at or above,” "at or below” or "at or above to at or
  // below” specified altitudes. The field is also used to
  // designate recommended altitudes and cases where two
  // distinct altitudes are provided at a single fix.
  // Source/Content: A code from the following table, selected
  // based on official government source or coding rules in
  // Attachment 5 and 6 to this document.
  // Field
  // Content Waypoint Crossing Description
  // + (plus)
  // "At or above” altitude specified in first "Altitude”
  // field. Also used with Localizer Only Altitude
  // field.
  // - (minus)
  // "At or below” altitude specified in first "Altitude”
  // field. Also used with Localizer Only Altitude
  // field.
  // A (same as @ or blank) "At” altitude specified in first "Altitude” field.
  // Also used with Localizer Only Altitude field.
  // B "At or above to at or below” altitudes specified in
  // the first and second "Altitude” fields.
  // C "At or above” altitude specified in second
  // "Altitude” field. Condition is which ever is
  // earlier.
  // D "At or above” altitude specified in second
  // "Altitude” field. Condition is "which ever is
  // later”, which is operationally equivalent to
  // the condition of "not before”.
  // G
  // Glide Slope altitude (MSL) specified in the
  // second "Altitude” field and "at” altitude specified
  // in the first "Altitude” field on the FAF Waypoint
  // in Precision Approach Coding with electronic
  // Glide Slope.
  // H
  // Glide Slope Altitude (MSL) specified in second
  // "Altitude” field and "at or above” altitude
  // specified in first "Altitude” field on the FAF
  // Waypoint in Precision Approach Coding with
  // electronic Glide Slope
  // I
  // Glide Slope Intercept Altitude specified in second
  // "Altitude” field and "at” altitude specified in first
  // "Altitude” field on the FACF Waypoint in
  // Precision Approach Coding with electronic Glide
  // Slope
  // J
  // Glide Slope Intercept Altitude specified in second
  // "Altitude” field and "at or above” altitude
  // specified in first "Altitude” field on the FACF
  // Waypoint in Precision Approach Coding with
  // electronic Glide Slope
  // V
  // "At” altitude on the coded vertical angle in the
  // second "Altitude” field and "at or above” altitude
  // specified in first "Altitude” field on step-down fix
  // waypoints.

  UNKNOWN = 0,
  A = 01, /* At altitude specified in first altitude field. */
  PLUS = 02, /* Fly at or above altitude1 */
  MINUS = 03, /* Fly at or below altitude1 */
  B = 04 /* At or above to at or below altitudes specified in the first and second "Altitude” fields. */
};

enum TurnDirection
{
  NONE = 0,
  L = 1,
  R = 2,
  BOTH = 3
};

enum SpeedDescriptor
{
  UNKNOWN_SPEED = 0,
  AT = 1,
  AT_OR_ABOVE = 2,
  AT_OR_BELOW = 3,
};

}
/*
 * Approach or transition leg. Not an actual record since it does not contain a header or size.
 */
class ApproachLeg
{
public:
  ApproachLeg(atools::io::BinaryStream *stream, atools::fs::bgl::rec::ApprRecordType recType);

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
   * Speed limit in knots. Assuming maximum speed.
   */
  float getSpeedLimit() const
  {
    return speedLimit;
  }

  float getVerticalAngle() const
  {
    return verticalAngle;
  }

  /*
   * @return true if this is a missed approach leg
   */
  bool isMissed() const
  {
    return missed;
  }

  /* true leg has valid values (type, course, etc. ) */
  bool isValid() const;

  static QString legTypeToString(atools::fs::bgl::leg::Type type);
  static QString legTypeToString(atools::fs::bgl::leg::Type type, const QString& src, bool warn);
  static QString altDescriptorToString(atools::fs::bgl::leg::AltDescriptor altDescr);
  static QString turnDirToString(atools::fs::bgl::leg::TurnDirection turnDir);
  static QString speedDescriptorToString(leg::SpeedDescriptor speedDescr); /* Only MSFS 2024 via SimConnect */

private:
  friend QDebug operator<<(QDebug out, const ApproachLeg& record);

  atools::fs::bgl::leg::Type type;
  atools::fs::bgl::leg::AltDescriptor altDescriptor;
  atools::fs::bgl::leg::TurnDirection turnDirection;
  atools::fs::bgl::ap::fix::ApproachFixType fixType, recommendedFixType;

  bool trueCourse, time, flyover, missed;
  QString fixIdent, fixRegion, fixAirportIdent, recommendedFixIdent, recommendedFixRegion;

  // "THETA” is defined as the
  // magnetic bearing to the waypoint
  // identified in the record’s "FIX
  // Ident” field from the NAVAID in the
  // "Recommended NAVAID” field
  // "RHO” is defined as the geodesic
  // distance in nautical miles to the
  // waypoint identified in the record’s
  // "Fix Ident” field from the NAVAID
  // in the "Recommended NAVAID”
  // field
  float theta /* heading */, rho /* distance */, course, distOrTime, altitude1, altitude2, speedLimit, verticalAngle;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTAPPROACHLEG_H
