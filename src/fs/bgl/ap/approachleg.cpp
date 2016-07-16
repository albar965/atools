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

#include "fs/bgl/ap/approachleg.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/ap/approach.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

ApproachLeg::ApproachLeg(io::BinaryStream *bs, bool ismissed)
{
  missed = ismissed;
  type = static_cast<leg::Type>(bs->readUByte());
  altDescriptor = static_cast<leg::AltDescriptor>(bs->readUByte());
  int flags = bs->readUShort();
  turnDirection = static_cast<leg::TurnDirection>(flags & 0x3);
  trueCourse = flags & (1 << 8);
  time = flags & (1 << 9);
  flyover = flags & (1 << 10);

  unsigned int fixFlags = bs->readUInt();
  fixType = static_cast<ap::fix::ApproachFixType>(fixFlags & 0xf);
  fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = bs->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  unsigned int recFixFlags = bs->readUInt();
  recommendedFixType = static_cast<ap::fix::ApproachFixType>(recFixFlags & 0xf);
  recommendedFixIdent = converter::intToIcao((recFixFlags >> 5) & 0xfffffff, true);
  recommendedFixRegion = converter::intToIcao(bs->readUInt() & 0x7ff, true); // TODO wiki mention mask

  theta = bs->readFloat(); // heading
  rho = bs->readFloat(); // distance
  course = bs->readFloat();
  distOrTime = bs->readFloat();
  altitude1 = bs->readFloat();
  altitude2 = bs->readFloat();
}

QString ApproachLeg::legTypeToString(leg::Type type)
{
  switch(type)
  {
    case atools::fs::bgl::leg::AF: // Arc To a Fix
      return "AF";

    case atools::fs::bgl::leg::CA: // Course To Altitude
      return "CA";

    case atools::fs::bgl::leg::CD: // Course To a DME
      return "CD";

    case atools::fs::bgl::leg::CF: // Course To a Fix
      return "CF";

    case atools::fs::bgl::leg::CI: // Course To Next Leg Intercept
      return "CI";

    case atools::fs::bgl::leg::CR: // Course To a Radial
      return "CR";

    case atools::fs::bgl::leg::DF: // Direct To a Fix
      return "DF";

    case atools::fs::bgl::leg::FA: // Fix To Altitude
      return "FA";

    case atools::fs::bgl::leg::FC: // Fix To a Distance on Course
      return "FC";

    case atools::fs::bgl::leg::FD: // Fix To a DME Termination
      return "FD";

    case atools::fs::bgl::leg::FM: // Fix To a Manual Termination
      return "FM";

    case atools::fs::bgl::leg::HA: // Hold to Altitude
      return "HA";

    case atools::fs::bgl::leg::HF: // Hold to Fix
      return "HF";

    case atools::fs::bgl::leg::HM: // Hold to Manual Termination
      return "HM";

    case atools::fs::bgl::leg::IF: // Initial Fix
      return "IF";

    case atools::fs::bgl::leg::PI: // Procedure Turn
      return "PI";

    case atools::fs::bgl::leg::RF: // Radius to Fix
      return "RF";

    case atools::fs::bgl::leg::TF: // Track To a Fix
      return "TF";

    case atools::fs::bgl::leg::VA: // Heading To Altitude
      return "VA";

    case atools::fs::bgl::leg::VD: // Heading To DME
      return "VD";

    case atools::fs::bgl::leg::VI: // Heading To Next Leg Intercept
      return "VI";

    case atools::fs::bgl::leg::VM: // Heading To Manual Termination
      return "VM";

    case atools::fs::bgl::leg::VR: // Heading To a Radial
      return "VR";

  }
  qWarning().nospace().noquote() << "Unknown approach leg type " << type;
  return QString();
}

QString ApproachLeg::altDescriptorToString(leg::AltDescriptor altDescr)
{
  switch(altDescr)
  {
    case atools::fs::bgl::leg::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::leg::A:
      return "A";

    case atools::fs::bgl::leg::PLUS:
      return "PLUS";

    case atools::fs::bgl::leg::MINUS:
      return "MINUS";

    case atools::fs::bgl::leg::B:
      return "B";

  }
  qWarning().nospace().noquote() << "Unknown approach altitude descriptor " << altDescr;
  return QString();
}

QString ApproachLeg::turnDirToString(leg::TurnDirection turnDir)
{
  switch(turnDir)
  {
    case atools::fs::bgl::leg::NONE:
      return "NONE";

    case atools::fs::bgl::leg::L:
      return "L";

    case atools::fs::bgl::leg::R:
      return "R";

    case atools::fs::bgl::leg::BOTH:
      return "BOTH";

  }
  qWarning().nospace().noquote() << "Unknown approach turn direction " << turnDir;
  return QString();
}

QDebug operator<<(QDebug out, const ApproachLeg& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " ApproachLeg["
  << "type " << ApproachLeg::legTypeToString(record.type)
  << ", alt descr " << ApproachLeg::altDescriptorToString(record.altDescriptor)
  << ", turn " << ApproachLeg::turnDirToString(record.turnDirection)
  << ", fix type " << ap::approachFixTypeToStr(record.fixType)
  << ", fix ident " << record.fixIdent
  << ", fix region " << record.fixRegion
  << ", fix airport " << record.fixAirportIdent
  << ", recommended fix type" << ap::approachFixTypeToStr(record.recommendedFixType)
  << ", recommended fix region " << record.recommendedFixRegion
  << ", theta (heading) " << record.theta
  << ", rho (distance) " << record.rho
  << ", course " << record.course
  << ", distOrTime " << record.distOrTime
  << ", altitude1 " << record.altitude1
  << ", altitude2 " << record.altitude2
  << ", trueCourse " << record.trueCourse
  << ", time " << record.time
  << ", flyover " << record.flyover
  << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
