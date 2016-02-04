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

#ifndef BGL_AIRPORTAPPROACHLEG_H_
#define BGL_AIRPORTAPPROACHLEG_H_

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
  AF = 0x01,
  CA = 0x02,
  CD = 0x03,
  CF = 0x04,
  CI = 0x05,
  CR = 0x06,
  DF = 0x07,
  FA = 0x08,
  FC = 0x09,
  FD = 0x0a,
  FM = 0x0b,
  HA = 0x0c,
  HF = 0x0d,
  HM = 0x0e,
  IF = 0x0f,
  PI = 0x10,
  RF = 0x11,
  TF = 0x12,
  VA = 0x13,
  VD = 0x14,
  VI = 0x15,
  VM = 0x16,
  VR = 0x17
};

enum AltDescriptor
{
  UNKNOWN=0,
  A = 01,
  PLUS = 02,
  MINUS = 03,
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

class ApproachLeg
{
public:
  ApproachLeg(atools::io::BinaryStream *bs, bool ismissed);

  static QString legTypeToString(atools::fs::bgl::leg::Type type);
  static QString altDescriptorToString(atools::fs::bgl::leg::AltDescriptor altDescr);
  static QString turnDirToString(atools::fs::bgl::leg::TurnDirection turnDir);

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

  atools::fs::bgl::ap::ApproachFixType getFixType() const
  {
    return fixType;
  }

  atools::fs::bgl::ap::ApproachFixType getRecommendedFixType() const
  {
    return recommendedFixType;
  }

  bool isTrueCourse() const
  {
    return trueCourse;
  }

  bool isTime() const
  {
    return time;
  }

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

  float getTheta() const
  {
    return theta;
  }

  float getRho() const
  {
    return rho;
  }

  float getCourse() const
  {
    return course;
  }

  float getDistOrTime() const
  {
    return distOrTime;
  }

  float getAltitude1() const
  {
    return altitude1;
  }

  float getAltitude2() const
  {
    return altitude2;
  }

  bool isMissed() const
  {
    return missed;
  }

private:
  friend QDebug operator<<(QDebug out, const ApproachLeg& record);

  atools::fs::bgl::leg::Type type;
  atools::fs::bgl::leg::AltDescriptor altDescriptor;
  atools::fs::bgl::leg::TurnDirection turnDirection;
  atools::fs::bgl::ap::ApproachFixType fixType, recommendedFixType;

  bool trueCourse, time, flyover, missed;
  QString fixIdent, fixRegion, fixAirportIdent, recommendedFixIdent, recommendedFixRegion;

  float theta /* heading */, rho /* distance */, course, distOrTime, altitude1, altitude2;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORTAPPROACHLEG_H_ */
