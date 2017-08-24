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

#include "fs/bgl/ap/airport.h"
#include "fs/bgl/recordtypes.h"
#include "fs/util/fsutil.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/navdatabaseoptions.h"
#include "fs/bgl/ap/jetway.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/util.h"
#include "fs/util/fsutil.h"
#include "atools.h"

#include <QDebug>
#include <QHash>
#include <QStringList>

namespace atools {
namespace fs {
namespace bgl {

struct ParkingKey
{
  int number;
  atools::fs::bgl::ap::ParkingName name;
};

bool operator==(const ParkingKey& k1, const ParkingKey& k2)
{
  return k1.name == k2.name && k1.number == k2.number;
}

bool operator!=(const ParkingKey& k1, const ParkingKey& k2)
{
  return !operator==(k1, k2);
}

inline uint qHash(const ParkingKey& pair)
{
  return static_cast<unsigned int>(pair.number) ^ static_cast<unsigned int>(pair.name);
}

using atools::io::BinaryStream;

Airport::Airport(const NavDatabaseOptions *options, BinaryStream *bs,
                 atools::fs::bgl::flags::CreateFlags flags)
  : Record(options, bs)
{
  Q_UNUSED(flags);

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

  bs->skip(4); // unknown, traffic scalar, unknown (FSX only)

  // FSX/FS9 structure recognition workaround
  // Check if the next record type is valid, if yes: FSX, otherwise it is a FS9 record where we have
  // to rewind 4 bytes
  Record tempRec(options, bs);
  rec::AirportRecordType tempType = tempRec.getId<rec::AirportRecordType>();
  if(!bgl::rec::airportRecordTypeValid(tempType))
  {
    qInfo() << "Found fs9 airport structure for" << ident;
    tempRec.seekToStart();
    bs->skip(-4);
  }
  else
    tempRec.seekToStart();

  QList<Jetway> jetways;
  QList<TaxiPoint> taxipoints;
  // Maps the number to index in array
  QHash<ParkingKey, int> parkingNumberIndex;
  QStringList taxinames;
  int helipadStart = 1;

  int subrecordIndex = 0;
  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::AirportRecordType type = r.getId<rec::AirportRecordType>();
    if(checkSubRecord(r))
      return;

    switch(type)
    {
      case rec::NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::RUNWAY_P3D_V4:
      case rec::RUNWAY:
        if(options->isIncludedNavDbObject(type::RUNWAY))
        {
          r.seekToStart();

          Runway rw = Runway(options, bs, ident, type == rec::RUNWAY_P3D_V4);
          if(!(options->isFilterRunways() &&
               rw.getLength() <= MIN_RUNWAY_LENGTH_METER && rw.getSurface() == bgl::rw::GRASS))
          {
            // append if it not a dummy runway

            if(!options->isFilterRunways() ||
               rw.getPosition().getPos().distanceMeterTo(getPosition().getPos()) < MAX_RUNWAY_DISTANCE_METER)
              // Omit all dummies that are far away from the airport center position
              runways.append(rw);
          }
        }
        break;
      case rec::COM:
        if(options->isIncludedNavDbObject(type::COM))
        {
          r.seekToStart();
          coms.append(Com(options, bs));
        }
        break;
      case rec::TAXI_PARKING_FS9: // FS9 parking has slightly different structure
      case rec::TAXI_PARKING:
        if(options->isIncludedNavDbObject(type::PARKING))
        {
          int numParkings = bs->readUShort();
          for(int i = 0; i < numParkings; i++)
          {
            Parking p(bs, type);

            // Remove vehicle parking later to avoid index mess-up

            parkings.append(p);
            parkingNumberIndex.insert({p.getNumber(), p.getName()}, parkings.size() - 1);
          }
        }
        break;
      case rec::APPROACH:
        if(options->isIncludedNavDbObject(type::APPROACH))
        {
          r.seekToStart();
          approaches.append(Approach(options, bs));
        }
        break;
      case rec::AIRPORT_WAYPOINT:
        if(options->isIncludedNavDbObject(type::WAYPOINT))
        {
          r.seekToStart();
          Waypoint wp(options, bs);

          if(wp.isValid())
            waypoints.append(wp);
          else
            qWarning() << "Found invalid record: " << wp.getObjectName();
        }
        break;
      case rec::DELETE_AIRPORT:
        r.seekToStart();
        deleteAirports.append(DeleteAirport(options, bs));
        break;

      case rec::APRON_FIRST:
        if(options->isIncludedNavDbObject(type::APRON))
        {
          r.seekToStart();
          aprons.append(Apron(options, bs));
        }
        break;
      case rec::APRON_SECOND_P3D_V4:
      case rec::APRON_SECOND:
        // if(options->includeBglObject(type::APRON2)) will be omitted when writing to the database
        r.seekToStart();
        aprons2.append(Apron2(options, bs, type == rec::APRON_SECOND_P3D_V4));
        break;
      case rec::APRON_EDGE_LIGHTS:
        if(options->isIncludedNavDbObject(type::APRON) && options->isIncludedNavDbObject(type::APRONLIGHT))
        {
          r.seekToStart();
          apronLights.append(ApronEdgeLight(options, bs));
        }
        break;
      case rec::HELIPAD:
        if(options->isIncludedNavDbObject(type::HELIPAD))
        {
          r.seekToStart();
          helipads.append(Helipad(options, bs));
        }
        break;
      case rec::START:
        if(options->isIncludedNavDbObject(type::START))
        {
          r.seekToStart();
          Start start(options, bs);
          if(start.getType() == atools::fs::bgl::start::HELIPAD)
            start.setNumber(helipadStart++);
          else
            start.setNumber(0);
          starts.append(start);
        }
        break;
      case rec::JETWAY:
        if(options->isIncludedNavDbObject(type::PARKING))
        {
          r.seekToStart();
          jetways.append(Jetway(options, bs));
        }
        break;
      case rec::FENCE_BOUNDARY:
        if(options->isIncludedNavDbObject(type::FENCE))
        {
          r.seekToStart();
          fences.append(Fence(options, bs));
        }
        numBoundaryFence++;
        break;
      case rec::FENCE_BLAST:
        if(options->isIncludedNavDbObject(type::FENCE))
        {
          r.seekToStart();
          fences.append(Fence(options, bs));
        }
        break;
      case rec::TOWER_OBJ:
        towerObj = true;
        break;
      case rec::TAXI_PATH_P3D_V4:
      case rec::TAXI_PATH:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          int numPaths = bs->readUShort();
          for(int i = 0; i < numPaths; i++)
          {
            TaxiPath path(bs, type == rec::TAXI_PATH_P3D_V4);
            if((path.getType() == atools::fs::bgl::taxipath::RUNWAY &&
                !options->isIncludedNavDbObject(type::TAXIWAY_RUNWAY)) ||
               (path.getType() == atools::fs::bgl::taxipath::VEHICLE &&
                !options->isIncludedNavDbObject(type::VEHICLE)))
              continue;

            taxipaths.append(path);
          }
        }
        break;
      case rec::TAXI_POINT:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          int numPoints = bs->readUShort();
          for(int i = 0; i < numPoints; i++)
            taxipoints.append(TaxiPoint(bs));
        }
        break;
      case rec::TAXI_NAME:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          int numNames = bs->readUShort();
          for(int i = 0; i < numNames; i++)
            taxinames.append(bs->readString(8)); // TODO fix wiki - first is always 0 and length always 8
        }
        break;
      case rec::UNKNOWN_REC:
        break;
      default:

        if(subrecordIndex == 0)
        {
          qWarning().nospace().noquote() << "Ignoring airport. Unexpected intial record type in Airport record 0x"
                                         << hex << type << dec << getObjectName();

          // Stop reading when the first subrecord is already invalid
          seekToStart();
          excluded = true;
          return;
        }
    }
    r.seekToEnd();
    subrecordIndex++;
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

  if(!options->isIncludedNavDbObject(type::VEHICLE))
    removeVehicleParking();

  // Update all the number fields and the bounding rectangle
  updateSummaryFields();

  if(deleteAirports.size() > 1)
    qWarning() << "Found more than one delete record in" << getObjectName();

  // TODO create warnings for this
  // Q_ASSERT(runways.size() == numRunways);
  // Q_ASSERT(approaches.size() == numApproaches);
  // Q_ASSERT(deleteAirports.size() == numDeleteRecords);
  // Q_ASSERT(coms.size() == numComs);
}

Airport::~Airport()
{
}

bool Airport::isEmpty() const
{
  return !towerObj &&
         name.isEmpty() &&
         runways.isEmpty() &&
         parkings.isEmpty() &&
         coms.isEmpty() &&
         helipads.isEmpty() &&
         starts.isEmpty() &&
         approaches.isEmpty() &&
         waypoints.isEmpty() &&
         deleteAirports.isEmpty() &&
         aprons.isEmpty() &&
         aprons2.isEmpty() &&
         apronLights.isEmpty() &&
         fences.isEmpty() &&
         taxipaths.isEmpty();
}

bool Airport::isNameMilitary(const QString& airportName)
{
  return atools::fs::util::isNameMilitary(airportName);
}

int Airport::calculateRating(bool isAddon) const
{
  // Maximum rating is 5
  return atools::fs::util::calculateAirportRating(isAddon,
                                                  hasTowerObj(), getTaxiPaths().size(), getParkings().size(),
                                                  getAprons().size());
}

bool Airport::isValid() const
{
  return !isEmpty() && position.getPos().isValid() && !position.getPos().isNull();
}

QString Airport::getObjectName() const
{
  return Record::getObjectName() + QString("airport ident %1 region %2 name %3 position %4").
         arg(ident).arg(region).arg(name).arg(position.getPos().toString());
}

void Airport::updateSummaryFields()
{
  boundingRect = atools::geo::Rect(getPosition().getPos());

  if(!towerPosition.getPos().isNull())
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
    // Use lowest frequency for default to have it deterministic
    if(c.getType() == com::TOWER && (towerFrequency == 0 || c.getFrequency() < towerFrequency))
      towerFrequency = c.getFrequency();
    else if(c.getType() == com::UNICOM && (unicomFrequency == 0 || c.getFrequency() < unicomFrequency))
      unicomFrequency = c.getFrequency();
    else if(c.getType() == com::AWOS && (awosFrequency == 0 || c.getFrequency() < awosFrequency))
      awosFrequency = c.getFrequency();
    else if(c.getType() == com::ASOS && (asosFrequency == 0 || c.getFrequency() < asosFrequency))
      asosFrequency = c.getFrequency();
    else if(c.getType() == com::ATIS && (atisFrequency == 0 || c.getFrequency() < atisFrequency))
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

  updateHelipads();

  military = isNameMilitary(name);
}

void Airport::updateParking(const QList<atools::fs::bgl::Jetway>& jetways,
                            const QHash<ParkingKey, int>& parkingNumberIndex)
{
  for(const Jetway& jw : jetways)
  {
    int index = parkingNumberIndex.value({jw.getParkingNumber(), jw.getGateName()}, -1);

    if(index != -1 && index < parkings.size())
    {
      if(parkings.at(index).jetway)
        qWarning().nospace().noquote() << "Parking for jetway " << jw << " already set" << dec
                                       << " for parking " << parkings.at(index)
                                       << " for ident " << ident;
      else
        parkings[index].jetway = true;
    }
    else
      qWarning().nospace().noquote() << "Parking for jetway " << jw << " not found" << dec
                                     << " for ident " << ident;
  }
}

void Airport::updateHelipads()
{
  for(Helipad& helipad : helipads)
  {
    int startIdx = 1;
    for(const Start& start : starts)
    {
      if(start.getPosition().getPos().almostEqual(helipad.getPosition().getPos(),
                                                  atools::geo::Pos::POS_EPSILON_5M))
        helipad.setStartIndex(startIdx);
      startIdx++;
    }
  }
}

void Airport::removeVehicleParking()
{
  QList<Parking>::iterator it = std::remove_if(parkings.begin(), parkings.end(),
                                               [](const Parking& p) -> bool
        {
          return p.getType() == atools::fs::bgl::ap::VEHICLES;
        });

  if(it != parkings.end())
    parkings.erase(it, parkings.end());
}

void Airport::updateTaxiPaths(const QList<TaxiPoint>& taxipoints, const QStringList& taxinames)
{
  using atools::inRange;

  for(TaxiPath& taxiPath : taxipaths)
  {
    switch(taxiPath.type)
    {
      case atools::fs::bgl::taxipath::UNKNOWN:
        break;
      case atools::fs::bgl::taxipath::PATH:
      case atools::fs::bgl::taxipath::CLOSED:
      case atools::fs::bgl::taxipath::TAXI:
      case atools::fs::bgl::taxipath::VEHICLE:
        if(inRange(taxinames, taxiPath.runwayNumTaxiName) && inRange(taxipoints, taxiPath.startPoint) &&
           inRange(taxipoints, taxiPath.endPoint))
        {
          taxiPath.taxiName = taxinames.at(taxiPath.runwayNumTaxiName);
          taxiPath.start = taxipoints.at(taxiPath.startPoint);
          taxiPath.end = taxipoints.at(taxiPath.endPoint);
        }
        else
          qWarning() << "One or more taxiway indexes out of bounds in" << ident
                     << "path type" << atools::fs::bgl::TaxiPath::pathTypeToString(taxiPath.type);
        break; // avoid fallthrough warning
      case atools::fs::bgl::taxipath::RUNWAY:
        if(inRange(taxipoints, taxiPath.startPoint) && inRange(taxipoints, taxiPath.endPoint))
        {
          taxiPath.start = taxipoints.at(taxiPath.startPoint);
          taxiPath.end = taxipoints.at(taxiPath.endPoint);
        }
        else
          qWarning() << "One or more taxiway indexes out of bounds in" << ident
                     << "path type" << atools::fs::bgl::TaxiPath::pathTypeToString(taxiPath.type);
        break;
      case atools::fs::bgl::taxipath::PARKING:
        if(inRange(taxinames, taxiPath.runwayNumTaxiName) && inRange(taxipoints, taxiPath.startPoint) &&
           inRange(parkings, taxiPath.endPoint))
        {
          taxiPath.taxiName = taxinames.at(taxiPath.runwayNumTaxiName);
          taxiPath.start = taxipoints.at(taxiPath.startPoint);
          taxiPath.end = TaxiPoint(parkings.at(taxiPath.endPoint));
        }
        else
          qWarning() << "One or more taxiway indexes out of bounds in" << ident
                     << "path type" << atools::fs::bgl::TaxiPath::pathTypeToString(taxiPath.type);
        break;
    }
  }

  // Remove all paths that remain invalid because of wrong indexes
  QList<TaxiPath>::iterator it = std::remove_if(taxipaths.begin(), taxipaths.end(),
                                                [](const TaxiPath& p) -> bool
        {
          return !p.getStartPoint().getPosition().isValid() ||
          !p.getEndPoint().getPosition().isValid();
        });

  if(it != taxipaths.end())
    taxipaths.erase(it, taxipaths.end());
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
