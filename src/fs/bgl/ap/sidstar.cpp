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

#include "fs/bgl/ap/approachleg.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/navdatabaseoptions.h"
#include "fs/bgl/ap/sidstar.h"

#include <QList>
#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

SidStar::SidStar(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  /* select the suffix based on if it's SID or STAR. */
  suffix = (rec::MSFS_SID == id) ? 'D' : 'A';
  stream->skip(2); /* skip 2 unknown bytes */
  (void)stream->readUByte(); /* runwayTransitionCt */
  (void)stream->readUByte(); /* commonRouteLegCt */
  (void)stream->readUByte(); /* enrouteTransitionCt */
  stream->skip(1); /* skip one more unknown byte */
  ident = stream->readString(8, atools::io::Encoding::UTF8);

  /*
   * The following are all sub records. There may be many runway records
   * too. We just need to collect up common route legs, which will get
   * applied to every STAR or SID, then all of the enroute transitions which
   * effectively represent transitions of the procedure. Finally, if there
   * aren't any runway transitions, the procedure applies to ALL runways;
   * otherwise, create one "approach" procedure per runway transition
   * sub record.
   *
   * It might be more efficient to read the child records iteratively.
   * For STARs, there will typically be 1 common route legs record,
   * followed by a number of runway transitions (which form each individual
   * STAR), and then a set of enroute transitions which apply to every STAR
   * as transitions.
   *
   * For SIDs, the process is the same. There will be optional common
   * route legs, followed by a number of runway transitions (which form each
   * individual SID), and finally a set of enroute transitions which apply
   * to every SID as transitions.
   */
  while(stream->tellg() < startOffset + size)
  {
    Record r(options, stream);
    rec::ApprRecordType recType = r.getId<rec::ApprRecordType>();
    if(checkSubRecord(r))
      return;

    switch(recType)
    {
      case rec::COMMON_ROUTE_LEGS_MSFS:
      case rec::COMMON_ROUTE_LEGS_MSFS_116:
      case rec::COMMON_ROUTE_LEGS_MSFS_118:
        {
          /*
           * The common route legs are similar to the base of an approach, except
           * these will be combined with each runway transition leg set to produce
           * an individual STAR (or SID?)
           *
           * NOTE: Unlike enroute and runway transitions, these legs are direct
           * children of the procedure!
           */
          int numLegs = stream->readUShort();
          for(int i = 0; i < numLegs; i++)
            commonRouteLegs.append(ApproachLeg(stream, recType));
        }
        break;

      case rec::RUNWAY_TRANSITIONS_MSFS:
        {
          /*
           * This will tell us the runway number and designator, as well as legs
           * leading to, or from said runway.
           */
          (void)stream->readUByte(); /* transitionCt */
          int runwayNumber = stream->readUByte();
          int runwayDesignator = stream->readUByte() & 0x7;
          /* Convert to a key */
          QString runwayName = converter::runwayToStr(runwayNumber, runwayDesignator);
          stream->skip(3);
          /* Create a container for the runway. */
          QList<ApproachLeg> legs;
          /* This will always be followed by 1 RUNWAY_TRANSITION_LEGS_MSFS(_116|_118) record */
          Record legRec(options, stream);
          int numLegs = stream->readUShort();
          for(int i = 0; i < numLegs; i++)
            legs.append(ApproachLeg(stream, legRec.getId<rec::ApprRecordType>()));

          /* And finally, we'll add it to our hash of runways. */
          runwayTransitionLegs.insert(runwayName, legs);
        }
        break;

      case rec::ENROUTE_TRANSITIONS_MSFS:
      case rec::ENROUTE_TRANSITIONS_MSFS_116:
        {
          /*
           * These are effectively the transitions for a SID/STAR. The normal Transition
           * class does quite understand these though...
           * Either we fix that, or modify the database writer... yuck.
           */
          QString name;
          (void)stream->readUByte(); /* transitionCt */
          stream->skip(1); /* unknown byte, usually zero */
          if(rec::ENROUTE_TRANSITIONS_MSFS_116 == recType)
          {
            /*
             * Note: The name field of the EnrouteTransitions record is now unused by
             * the current version of MSFS 2020 SDK, and hence all of its bytes are left NULL.
             * All transitions are named after their first (STAR) or last (SID) leg.
             *
             * In fact, a XML file containing <EnrouteTransitions name="SAVLA"> is not compiled
             * compiled by the SDK anymore. The SDK registers it as an invalid field error.
             *
             * Therefore, these 8 bytes can be skipped.
             */
            (void)stream->readString(8, atools::io::UTF8);
          }
          /* Create a container for the transition legs */
          QList<ApproachLeg> legs;
          /* This will always be followed by 1 ENROUTE_TRANSITION_LEGS_MSFS(_116|_118) record */
          Record legRec(options, stream);
          int numLegs = stream->readUShort();
          for(int i = 0; i < numLegs; i++)
            legs.append(ApproachLeg(stream, legRec.getId<rec::ApprRecordType>()));

          if(!legs.isEmpty())
          {
            /* Get the ident (key) of the transition. Must always be done this way. */
            if(rec::MSFS_SID == id)
              /* For SID, the transition ident is the LAST leg's fix. */
              name = legs.constLast().getFixIdent();
            else if(rec::MSFS_STAR == id)
              /* For STAR, the transition ident is the FIRST leg's fix */
              name = legs.constFirst().getFixIdent();

            enrouteTransitions.insert(name, legs);
          }
          else
            qWarning() << Q_FUNC_INFO << "No enroute transition found" << getDescription();
        }
        break;

      default:
        /* Shouldn't ever occur, so print error and move on? */
        qWarning().noquote().nospace() << Q_FUNC_INFO << " Unexpected record type " << ident
                                       << Qt::hex << " 0x" << r.getId()
                                       << Qt::dec << " " << approachRecordTypeStr(recType) << " offset " << stream->tellg();

    }
    r.seekToEnd();
  }
}

SidStar::~SidStar()
{
}

QDebug operator<<(QDebug out, const SidStar& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << QString((rec::MSFS_SID == record.id) ? " Departure" : " Arrival")
                          << "[ident " << record.ident
                          << ", " << Qt::endl;
  out << "  commonRouteLegs " << record.commonRouteLegs;
  out << "  runwayTransitionLegs " << record.runwayTransitionLegs;
  out << "  enrouteTransitions " << record.enrouteTransitions;
  out << "]";
  return out;
}

bool SidStar::isValid() const
{
  // Need at least one leg
  bool valid = commonRouteLegs.size() + runwayTransitionLegs.size() + enrouteTransitions.size() > 0;

  for(const ApproachLeg& leg: std::as_const(commonRouteLegs))
    valid &= leg.isValid();

  for(const QList<atools::fs::bgl::ApproachLeg>& legs : std::as_const(enrouteTransitions))
  {
    for(const ApproachLeg& leg: legs)
      valid &= leg.isValid();
  }

  for(const QList<atools::fs::bgl::ApproachLeg>& legs : std::as_const(runwayTransitionLegs))
  {
    for(const ApproachLeg& leg: legs)
      valid &= leg.isValid();
  }

  return valid;

}

QString SidStar::getDescription() const
{
  return QString((rec::MSFS_SID == id) ? "Departure" : "Arrival") +
         "[ident " + ident + "]";
}

} // namespace bgl
} // namespace fs
} // namespace atools
