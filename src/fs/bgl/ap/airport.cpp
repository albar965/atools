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

#include "logging/loggingdefs.h"

#include "fs/bgl/ap/airport.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/navdatabaseoptions.h"
#include "fs/bgl/ap/jetway.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/util.h"

#include <QHash>
#include <QStringList>

namespace atools {
namespace fs {
namespace bgl {

static const QStringList MIL_ENDS_WITH(" Mil");
static const QStringList MIL_CONTAINS({" AAF", " AB", " AF", " AFB", " AFS", " AHP", " ANGB", " ARB", " LRRS",
                                       " MCAF", " MCALF", " MCAS", " NAF", " NALF", " NAS", " Naval", " Navy",
                                       " NAWS", " NOLF", " NS", " Army", " Mil ", "Military", "Air Force"});

using atools::io::BinaryStream;

Airport::Airport(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  /*int numRunways = TODO compare with number of subrecords */
  bs->readUByte();
  /*int numComs = TODO compare with number of subrecords*/ bs->readUByte();
  bs->readUByte(); // numStarts
  /*int numApproaches = TODO compare with number of subrecords*/ bs->readUByte();
  /*int numAprons = TODO compare with number of subrecords*/ bs->readUByte();
  // int numDeleteRecords = (numAprons & 0x80) == 0x80; TODO compare with delete record presence
  bs->readUByte(); // numHelipads TODO compare with number of subrecords
  position = BglPosition(bs, true, 1000.f);
  towerPosition = BglPosition(bs, true, 1000.f);
  magVar = converter::adjustMagvar(bs->readFloat());
  ident = converter::intToIcao(bs->readUInt());

  // Check if the airport is filtered out in the configuration file
  if(!options->isIncludedAirportIdent(ident))
  {
    // Stop reading
    seekToStart();
    excluded = true;
    return;
  }

  region = converter::intToIcao(bs->readUInt()); // TODO wiki is always null
  fuelFlags = static_cast<ap::FuelFlags>(bs->readUInt());

  bs->skip(4); // unknown, traffic scalar, unknown

  QList<Jetway> jetways;
  QList<TaxiPoint> taxipoints;
  QHash<int, int> parkingNumberIndex;
  QStringList taxinames;

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::AirportRecordType t = r.getId<rec::AirportRecordType>();

    switch(t)
    {
      case rec::NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::RUNWAY:
        if(options->isIncludedBglObject(type::RUNWAY))
        {
          r.seekToStart();

          Runway rw = Runway(options, bs, ident);
          if(!(options->isFilterRunways() &&
               rw.getLength() <= MIN_RUNWAY_LENGTH && rw.getSurface() == bgl::rw::GRASS))
            // append if it not a dummy runway
            runways.append(rw);
        }
        break;
      case rec::COM:
        if(options->isIncludedBglObject(type::COM))
        {
          r.seekToStart();
          coms.append(Com(options, bs));
        }
        break;
      case rec::TAXI_PARKING:
        if(options->isIncludedBglObject(type::PARKING))
        {
          int numParkings = bs->readUShort();
          for(int i = 0; i < numParkings; i++)
          {
            Parking p(bs);
            parkings.append(p);
            parkingNumberIndex.insert(p.getNumber(), parkings.size() - 1);
          }
        }
        break;
      case rec::APPROACH:
        if(options->isIncludedBglObject(type::APPROACH))
        {
          r.seekToStart();
          approaches.append(Approach(options, bs));
        }
        break;
      case rec::AIRPORT_WAYPOINT:
        if(options->isIncludedBglObject(type::WAYPOINT))
        {
          r.seekToStart();
          waypoints.append(Waypoint(options, bs));
        }
        break;
      case rec::DELETE_AIRPORT:
        r.seekToStart();
        deleteAirports.append(DeleteAirport(options, bs));
        break;

      case rec::APRON_FIRST:
        if(options->isIncludedBglObject(type::APRON))
        {
          r.seekToStart();
          aprons.append(Apron(options, bs));
        }
        break;
      case rec::APRON_SECOND:
        // if(options->includeBglObject(type::APRON2)) will be omitted when writing to the database
        r.seekToStart();
        aprons2.append(Apron2(options, bs));
        break;
      case rec::APRON_EDGE_LIGHTS:
        if(options->isIncludedBglObject(type::APRON) && options->isIncludedBglObject(type::APRONLIGHT))
        {
          r.seekToStart();
          apronLights.append(ApronEdgeLight(options, bs));
        }
        break;
      case rec::HELIPAD:
        if(options->isIncludedBglObject(type::HELIPAD))
        {
          r.seekToStart();
          helipads.append(Helipad(options, bs));
        }
        break;
      case rec::START:
        if(options->isIncludedBglObject(type::START))
        {
          r.seekToStart();
          starts.append(Start(options, bs));
        }
        break;
      case rec::JETWAY:
        if(options->isIncludedBglObject(type::PARKING))
        {
          r.seekToStart();
          jetways.append(Jetway(options, bs));
        }
        break;
      case rec::FENCE_BOUNDARY:
        if(options->isIncludedBglObject(type::FENCE))
        {
          r.seekToStart();
          fences.append(Fence(options, bs));
        }
        numBoundaryFence++;
        break;
      case rec::FENCE_BLAST:
        if(options->isIncludedBglObject(type::FENCE))
        {
          r.seekToStart();
          fences.append(Fence(options, bs));
        }
        break;
      case rec::TOWER_OBJ:
        towerObj = true;
        break;
      case rec::TAXI_PATH:
        if(options->isIncludedBglObject(type::TAXIWAY))
        {
          int numPaths = bs->readUShort();
          for(int i = 0; i < numPaths; i++)
          {
            TaxiPath path(bs);
            if((path.getType() == atools::fs::bgl::taxipath::RUNWAY &&
                !options->isIncludedBglObject(type::TAXIWAY_RUNWAY)) ||
               (path.getType() == atools::fs::bgl::taxipath::VEHICLE &&
                !options->isIncludedBglObject(type::TAXIWAY_VEHICLE)))
              continue;

            taxipaths.append(path);
          }
        }
        break;
      case rec::TAXI_POINT:
        if(options->isIncludedBglObject(type::TAXIWAY))
        {
          int numPoints = bs->readUShort();
          for(int i = 0; i < numPoints; i++)
            taxipoints.append(TaxiPoint(bs));
        }
        break;
      case rec::TAXI_NAME:
        if(options->isIncludedBglObject(type::TAXIWAY))
        {
          int numNames = bs->readUShort();
          for(int i = 0; i < numNames; i++)
            taxinames.append(bs->readString(8));  // TODO fix wiki - first is always 0 and length always 8
        }
        break;
      case rec::UNKNOWN_REC:
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in Airport record 0x" << hex << t << dec
                                       << " for ident " << ident;
    }
    r.seekToEnd();
  }

  // Disabled since dummies for Unlisted_Airstrips are all empty
  // if(runways.isEmpty() && deleteAirports.isEmpty() && parkings.isEmpty() && taxipaths.isEmpty() &&
  // aprons.isEmpty() && coms.isEmpty() && helipads.isEmpty() && starts.isEmpty())
  // {
  // seekToStart();
  // excluded = true;
  // return;
  // }

  // Add start, end points and names to taxi paths
  updateTaxiPaths(taxipoints, taxinames);

  // Set the jetway flag on parking
  updateParking(jetways, parkingNumberIndex);

  // Update all the number fields and the bounding rectange
  updateSummaryFields();

  // TODO create warnings for this
  // Q_ASSERT(runways.size() == numRunways);
  // Q_ASSERT(approaches.size() == numApproaches);
  // Q_ASSERT(deleteAirports.size() == numDeleteRecords);
  // Q_ASSERT(coms.size() == numComs);
}

Airport::~Airport()
{
}

void Airport::updateSummaryFields()
{
  boundingRect = atools::geo::Rect(getPosition().getPos());
  if(towerPosition.getLatY() != 0.f && towerPosition.getLonX() != 0.f)
    boundingRect.extend(towerPosition.getPos());

  for(const Runway& rw : runways)
  {
    // Count runway types
    if(rw.getEdgeLight() != rw::NO_LIGHT)
      numLightRunway++;
    if(rw.isHard())
      numHardRunway++;
    if(rw.isWater())
      numWaterRunway++;
    if(rw.isSoft())
      numSoftRunway++;

    // Extend bounding rectangle for runway dimensions
    boundingRect.extend(rw.getPosition().getPos());
    boundingRect.extend(rw.getSecondaryPosition());
    boundingRect.extend(rw.getPrimaryPosition());

    // Remember the longest runway attributes
    if(rw.getLength() > longestRunwayLength)
    {
      longestRunwayLength = rw.getLength();
      longestRunwayWidth = rw.getWidth();
      longestRunwayHeading = rw.getHeading();
      longestRunwaySurface = rw.getSurface();
    }

    const RunwayEnd& primary = rw.getPrimary();
    const RunwayEnd& secondary = rw.getSecondary();

    if(primary.getApproachLights().getSystem() != rw::NO_ALS)
      numRunwayEndApproachLight++;
    if(secondary.getApproachLights().getSystem() != rw::NO_ALS)
      numRunwayEndApproachLight++;

    if(primary.getLeftVasi().getType() != rw::NONE || primary.getRightVasi().getType() != rw::NONE)
      numRunwayEndVasi++;
    if(secondary.getLeftVasi().getType() != rw::NONE || secondary.getRightVasi().getType() != rw::NONE)
      numRunwayEndVasi++;

    if(bgl::util::isFlagSet(rw.getMarkingFlags(), rw::PRIMARY_CLOSED))
      numRunwayEndClosed++;
    if(bgl::util::isFlagSet(rw.getMarkingFlags(), rw::SECONDARY_CLOSED))
      numRunwayEndClosed++;
  }

  // If all runways are closed the airport is closed
  airportClosed = !runways.isEmpty() && numRunwayEndClosed / 2 == runways.size();

  for(const Com& c : coms)
  {
    if(c.getType() == com::TOWER)
      towerFrequency = c.getFrequency();
    else if(c.getType() == com::UNICOM)
      unicomFrequency = c.getFrequency();
    else if(c.getType() == com::AWOS)
      awosFrequency = c.getFrequency();
    else if(c.getType() == com::ASOS)
      asosFrequency = c.getFrequency();
    else if(c.getType() == com::ATIS)
      atisFrequency = c.getFrequency();
  }

  for(const Parking& p : parkings)
  {
    boundingRect.extend(p.getPosition().getPos());

    if(p.hasJetway())
      numJetway++;

    if(p.isGate())
    {
      numParkingGate++;

      if(largestParkingGate == ap::UNKNOWN || largestParkingGate < p.getType())
        largestParkingGate = p.getType();
    }

    if(p.isGaRamp())
    {
      numParkingGaRamp++;
      if(largestParkingGaRamp == ap::UNKNOWN || largestParkingGaRamp < p.getType())
        largestParkingGaRamp = p.getType();
    }

    if(p.getType() == ap::RAMP_CARGO)
      numParkingCargo++;

    if(p.getType() == ap::RAMP_MIL_CARGO)
      numParkingMilitaryCargo++;

    if(p.getType() == ap::RAMP_MIL_COMBAT)
      numParkingMilitaryCombat++;
  }

  for(const Fence& f : fences)
  {
    for(const BglPosition& p : f.getVertices())
      boundingRect.extend(p.getPos());
  }

  for(const Apron& a : aprons)
    for(const BglPosition &p : a.getVertices())
      boundingRect.extend(p.getPos());

  for(const Apron2& a : aprons2)
    for(const BglPosition &p : a.getVertices())
      boundingRect.extend(p.getPos());

  for(const Start& s : starts)
    boundingRect.extend(s.getPosition().getPos());

  for(const Helipad& h : helipads)
    boundingRect.extend(h.getPosition().getPos());

  for(const TaxiPath& p : taxipaths)
  {
    boundingRect.extend(p.getStartPoint().getPosition().getPos());
    boundingRect.extend(p.getEndPoint().getPosition().getPos());
  }

  // Check if airport is military
  for(const QString& s : MIL_ENDS_WITH)
    if(name.endsWith(s))
    {
      military = true;
      break;
    }

  if(!military)
    for(const QString& s : MIL_CONTAINS)
      if(name.contains(s))
      {
        military = true;
        break;
      }

}

void Airport::updateParking(const QList<Jetway>& jetways, const QHash<int, int>& parkingNumberIndex)
{
  for(const Jetway& jw : jetways)
  {
    QHash<int, int>::const_iterator iter = parkingNumberIndex.find(jw.getParkingNumber());
    if(iter != parkingNumberIndex.end())
      parkings[*iter].jetway = true;
    else
      qWarning().nospace().noquote() << "Parking for jetway " << jw << " not found" << dec
                                     << " for ident " << ident;
  }
}

void Airport::updateTaxiPaths(const QList<TaxiPoint>& taxipoints, const QStringList& taxinames)
{
  for(TaxiPath& t : taxipaths)
  {
    switch(t.type)
    {
      case atools::fs::bgl::taxipath::UNKNOWN :
        break;
      case atools::fs::bgl::taxipath::PATH:
      case atools::fs::bgl::taxipath::CLOSED:
      case atools::fs::bgl::taxipath::TAXI:
      case atools::fs::bgl::taxipath::VEHICLE:
        t.taxiName = taxinames.at(t.runwayNumTaxiName);
        t.start = taxipoints.at(t.startPoint);
        t.end = taxipoints.at(t.endPoint);
        break; // avoid fallthrough warning
      case atools::fs::bgl::taxipath::RUNWAY:
        t.start = taxipoints.at(t.startPoint);
        t.end = taxipoints.at(t.endPoint);
        break;
      case atools::fs::bgl::taxipath::PARKING:
        t.taxiName = taxinames.at(t.runwayNumTaxiName);
        t.start = taxipoints.at(t.startPoint);
        t.end = TaxiPoint(parkings.at(t.endPoint));
        break;
    }
  }
}

QDebug operator<<(QDebug out, const Airport& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Airport[ICAO " << record.ident
  << ", name " << record.name
  << ", region " << record.region
  << ", " << record.position.getPos()
  << ", magvar " << record.magVar << ", " << endl;
  out << record.runways;
  out << record.coms;
  out << record.aprons;
  out << record.aprons2;
  out << record.apronLights;
  out << record.approaches;
  out << record.parkings;
  out << record.deleteAirports;
  out << record.helipads;
  out << record.starts;
  out << record.fences;
  out << record.taxipaths;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
