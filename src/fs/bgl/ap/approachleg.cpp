/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
#include "atools.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

ApproachLeg::ApproachLeg(io::BinaryStream *stream, rec::ApprRecordType recType)
{
  missed = recType == rec::MISSED_LEGS || recType == rec::MISSED_LEGS_MSFS || recType == rec::MISSED_LEGS_MSFS_116 ||
           recType == rec::MISSED_LEGS_MSFS_118;

  type = static_cast<leg::Type>(stream->readUByte());

  altDescriptor = static_cast<leg::AltDescriptor>(stream->readUByte());
  int flags = stream->readUShort();
  turnDirection = static_cast<leg::TurnDirection>(flags & 0x3);
  trueCourse = flags & (1 << 8);
  time = flags & (1 << 9);
  flyover = flags & (1 << 10);

  unsigned int fixFlags = stream->readUInt();
  fixType = static_cast<ap::fix::ApproachFixType>(fixFlags & 0xf);
  fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = stream->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  unsigned int recFixFlags = stream->readUInt();
  recommendedFixType = static_cast<ap::fix::ApproachFixType>(recFixFlags & 0xf);
  recommendedFixIdent = converter::intToIcao((recFixFlags >> 5) & 0xfffffff, true);
  recommendedFixRegion = converter::intToIcao(stream->readUInt() & 0x7ff, true);

  theta = stream->readFloat(); // heading
  rho = stream->readFloat(); // distance
  course = stream->readFloat();
  distOrTime = stream->readFloat();
  altitude1 = stream->readFloat();
  altitude2 = stream->readFloat();

  // Determine type by using record id =============================
  // Common MSFS records
  bool msfs = rec::approachRecordTypeMsfs(recType);

  // New MSFS records since 1.16.1 ======
  bool msfs116 = rec::approachRecordTypeMsfs116(recType);

  // New MSFS records since 1.18.9 ======
  bool msfs118 = rec::approachRecordTypeMsfs118(recType);

  if(msfs)
  {
    // Not type given - assuming max speed
    speedLimit = stream->readFloat();
    verticalAngle = stream->readFloat();
    stream->skip(8); // unknown

    if(msfs116 || msfs118)
    {
      // New MSFS structures
      // Check for constant radius turn legs - these need the center point in the recommended fix
      if(type == leg::RF)
      {
        // if(!recommendedFixIdent.isEmpty())
        // qWarning() << Q_FUNC_INFO << "Recommended fix overlap in RF leg"
        // << recommendedFixIdent << "/" << recommendedFixRegion;

        // TODO create separate center columns in database
        // The recommended fix is used as the arc center navaid in the LNM database due to historical reasons
        // The original recommended fix has to be wiped out here which is not an
        // issue since this overlaps only in a dozen or so cases
        recFixFlags = stream->readUInt();
        recommendedFixType = static_cast<ap::fix::ApproachFixType>(recFixFlags & 0xf);
        recommendedFixIdent = converter::intToIcao((recFixFlags >> 5) & 0xfffffff, true);
        recommendedFixRegion = converter::intToIcao(stream->readUInt() & 0x7ff, true);
      }
      else
        // Skip center fix data
        stream->skip(8);
    }
  }

  if(msfs118)
    // Unknown data
    stream->skip(4);
}

QString ApproachLeg::legTypeToString(leg::Type type)
{
  return legTypeToString(type, QString(), false);
}

QString ApproachLeg::legTypeToString(leg::Type type, const QString& src, bool warn)
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
  if(warn)
    qWarning().nospace().noquote() << "Invalid approach leg type " << type << " Msg: " << src;
  return "INVALID";
}

QString ApproachLeg::altDescriptorToString(leg::AltDescriptor altDescr)
{
  switch(altDescr)
  {
    case atools::fs::bgl::leg::UNKNOWN:
      return QString();

    case atools::fs::bgl::leg::A:
      return "A";

    case atools::fs::bgl::leg::PLUS:
      return "+";

    case atools::fs::bgl::leg::MINUS:
      return "-";

    case atools::fs::bgl::leg::B:
      return "B";

  }
  qWarning().nospace().noquote() << "Invalid approach altitude descriptor " << altDescr;
  return "INVALID";
}

QString ApproachLeg::speedDescriptorToString(leg::SpeedDescriptor speedDescr)
{
  switch(speedDescr)
  {
    case leg::UNKNOWN_SPEED:
    case leg::AT:
      return QString();

    case leg::AT_OR_ABOVE:
      return "+";

    case leg::AT_OR_BELOW:
      return "-";
  }
  qWarning().nospace().noquote() << "Invalid approach altitude descriptor " << speedDescr;
  return "INVALID";
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
      return "B";

  }
  qWarning().nospace().noquote() << "Invalid approach turn direction " << turnDir;
  return "INVALID";
}

bool ApproachLeg::isValid() const
{
  // Check validity by looking at the most important fields in the leg
  return legTypeToString(type, QString(), false) != "INVALID" && altDescriptorToString(altDescriptor) != "INVALID" &&
         turnDirToString(turnDirection) != "INVALID" && atools::inRange(0.f, 360.f, theta) &&
         atools::inRange(0.f, 360.f, course) &&
         rho >= 0.f &&
         atools::inRange(0.f, 20000.f, altitude1) &&
         atools::inRange(0.f, 20000.f, altitude2);
}

QDebug operator<<(QDebug out, const ApproachLeg& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " ApproachLeg["
                          << "type " << ApproachLeg::legTypeToString(record.type, QString(), false)
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
