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

#include "atools.h"
#include "fs/bgl/ap/airport.h"
#include "fs/bgl/ap/jetway.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/util/fsutil.h"
#include "fs/util/fsutil.h"
#include "geo/calculations.h"
#include "geo/linestring.h"
#include "io/binarystream.h"

#include <QDebug>
#include <QHash>
#include <QStringList>

namespace atools {
namespace fs {
namespace bgl {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
using Qt::endl;
#endif

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

Airport::Airport(const NavDatabaseOptions *options, BinaryStream *stream, atools::fs::bgl::CreateFlags flags)
  : Record(options, stream)
{
  /*int numRunways = TODO compare with number of subrecords */
  stream->readUByte();
  /*int numComs = TODO compare with number of subrecords*/ stream->readUByte();
  stream->readUByte(); // numStarts
  /*int numApproaches = TODO compare with number of subrecords*/ stream->readUByte();
  /*int numAprons = TODO compare with number of subrecords*/ stream->readUByte();
  // int numDeleteRecords = (numAprons & 0x80) == 0x80; TODO compare with delete record presence
  stream->readUByte(); // numHelipads TODO compare with number of subrecords
  position = BglPosition(stream, true, 1000.f);
  towerPosition = BglPosition(stream, true, 1000.f);
  magVar = converter::adjustMagvar(stream->readFloat());
  ident = converter::intToIcao(stream->readUInt());
  msfs = options->getSimulatorType() == atools::fs::FsPaths::SimulatorType::MSFS;

  // Check if the airport is filtered out in the configuration file
  if(!options->isIncludedAirportIdent(ident))
  {
    // Stop reading
    seekToStart();
    excluded = true;
    return;
  }

  region = converter::intToIcao(stream->readUInt()); // TODO wiki is always null

  fuelFlags = static_cast<ap::FuelFlags>(stream->readUInt());

  if(msfs)
  {
    stream->skip(2);
    quint8 msfsflags = stream->readUByte();
    if((msfsflags & 0x04) == 0x04)
      airportClosed = true;
    if((msfsflags & 0x01) == 0x01)
      msfsStar = true;
    stream->skip(1);
  }
  else
    stream->skip(4); // unknown, traffic scalar, unknown (FSX only)

  if(msfs)
  {
    /*int numDepartures = TODO compare with number of departure subrecords*/
    stream->readUByte();
    stream->skip(1);
    /*int numArrivals = TODO compare with number of arrival subrecords*/ stream->readUByte();
    stream->skip(9); /* skip applyFlatten, and 4 16-bit counts */
  }
  else if(!isCurrentRecordValid())
  {
    // FSX/FS9 structure recognition workaround
    // Check if the next record type is valid, if yes: FSX, otherwise it is a FS9 record where we have
    // to rewind 4 bytes
    stream->skip(-4);

    if(!isCurrentRecordValid())
      // Still not valid - must be P3Dv5 structure
      stream->skip(8);
    else
      qInfo() << "Found fs9 airport structure for" << ident;
  }

  QList<Jetway> jetways;
  QList<TaxiPoint> taxipoints;
  // Maps the number to index in array
  QHash<ParkingKey, int> parkingNumberIndex;
  QStringList taxinames;
  int helipadStart = 1;
  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  int subrecordIndex = 0;
  while(stream->tellg() < startOffset + size)
  {
    Record r(options, stream);
    rec::AirportRecordType type = r.getId<rec::AirportRecordType>();
    if(checkSubRecord(r))
    {
      qWarning().noquote().nospace() << Q_FUNC_INFO << "Invalid record" << hex << " 0x" << r.getId()
                                     << dec << " " << airportRecordTypeStr(type) << " offset " << stream->tellg();
      return;
    }

    // qDebug().nospace() << Q_FUNC_INFO << hex << " 0x" << r.getId()
    // << dec << " " << airportRecordTypeStr(type) << " " << bs->tellg();

    switch(type)
    {
      case rec::NAME:
        name = stream->readString(r.getSize() - Record::SIZE, encoding);
        break;

      case rec::RUNWAY:
      case rec::RUNWAY_P3D_V4:
      case rec::RUNWAY_MSFS:
        if(options->isIncludedNavDbObject(type::RUNWAY))
        {
          r.seekToStart();

          StructureType structureType = STRUCT_FSX;
          if(type == rec::RUNWAY_P3D_V4)
            structureType = STRUCT_P3DV4;
          else if(type == rec::RUNWAY_MSFS)
            structureType = STRUCT_MSFS;

          Runway rw = Runway(options, stream, ident, structureType);
          if(!(options->isFilterRunways() && rw.getLength() <= MIN_RUNWAY_LENGTH_METER &&
               rw.getSurface() == bgl::GRASS))
          {
            // append if it not a dummy runway
            if(!options->isFilterRunways() ||
               rw.getPosition().getPos().distanceMeterTo(getPos()) < MAX_RUNWAY_DISTANCE_METER)
              // Omit all dummies that are far away from the airport center position
              runways.append(rw);
          }
        }
        break;

      case rec::COM:
        if(options->isIncludedNavDbObject(type::COM))
        {
          r.seekToStart();
          Com com(options, stream);

          if(com.getFrequency() > 0) // MSFS has zero frequency COM values
            coms.append(com);
        }
        break;

      case rec::TAXI_PARKING_P3D_V5:
      case rec::TAXI_PARKING_FS9: // FS9 parking has slightly different structure
      case rec::TAXI_PARKING:
      case rec::TAXI_PARKING_MSFS:
        if(options->isIncludedNavDbObject(type::PARKING))
        {
          StructureType structType = STRUCT_FSX;
          if(type == rec::TAXI_PARKING_P3D_V5)
            structType = STRUCT_P3DV5;
          else if(type == rec::TAXI_PARKING_FS9)
            structType = STRUCT_FS9;
          else if(type == rec::TAXI_PARKING_MSFS)
            structType = STRUCT_MSFS;

          int numParkings = stream->readUShort();
          for(int i = 0; i < numParkings; i++)
          {
            Parking parking(stream, structType);

#ifdef DEBUG_INFORMATION_TAXI
            if(!parking.isValid())
              qWarning() << Q_FUNC_INFO << "Invalid parking point #" << i << ident;
#endif

            // Remove vehicle parking later to avoid index mess-up
            parkings.append(parking);
            parkingNumberIndex.insert({parking.getNumber(), parking.getName()}, parkings.size() - 1);
          }
        }
        break;

      case rec::APPROACH:
      case rec::MSFS_APPROACH_NEW:
        if(options->isIncludedNavDbObject(type::APPROACH) && !flags.testFlag(atools::fs::bgl::AIRPORT_MSFS_NAVIGRAPH_NAVDATA))
        {
          r.seekToStart();
          approaches.append(Approach(options, stream, type));
        }
        break;

      case rec::MSFS_SID:
      case rec::MSFS_STAR:
        if(options->isIncludedNavDbObject(type::APPROACH) && !flags.testFlag(atools::fs::bgl::AIRPORT_MSFS_NAVIGRAPH_NAVDATA))
        {
          r.seekToStart();
          sidsAndStars.append(SidStar(options, stream));
        }
        break;

      case rec::AIRPORT_WAYPOINT:
        if(options->isIncludedNavDbObject(type::WAYPOINT))
        {
          r.seekToStart();
          Waypoint wp(options, stream);

          if(wp.isValid())
            waypoints.append(wp);
          else
            qWarning() << Q_FUNC_INFO << "Found invalid record: " << wp.getObjectName();
        }
        break;

      case rec::DELETE_AIRPORT:
        r.seekToStart();
        deleteAirports.append(DeleteAirport(options, stream));
        break;

      case rec::APRON_FIRST_MSFS:
      case rec::APRON_FIRST_P3D_V5:
      case rec::APRON_FIRST:
      case rec::APRON_FIRST_MSFS_NEW:
        if(options->isIncludedNavDbObject(type::APRON))
        {
          r.seekToStart();
          aprons.append(Apron(options, stream, type));
        }
        break;

      case rec::APRON_SECOND_P3D_V5:
      case rec::APRON_SECOND_P3D_V4:
      case rec::APRON_SECOND:
        {
          r.seekToStart();
          StructureType structType = STRUCT_FSX;
          if(type == rec::APRON_SECOND_P3D_V5)
            structType = STRUCT_P3DV5;
          else if(type == rec::APRON_SECOND_P3D_V4)
            structType = STRUCT_P3DV4;
          aprons2.append(Apron2(options, stream, structType));
        }
        break;

      case rec::HELIPAD:
        if(options->isIncludedNavDbObject(type::HELIPAD))
        {
          r.seekToStart();
          helipads.append(Helipad(options, stream));
        }
        break;

      case rec::START:
        if(options->isIncludedNavDbObject(type::START))
        {
          r.seekToStart();
          Start start(options, stream);
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
          jetways.append(Jetway(options, stream));
        }
        break;

      case rec::TOWER_OBJ:
        towerObj = true;
        break;

      case rec::TAXI_PATH_P3D_V5:
      case rec::TAXI_PATH_P3D_V4:
      case rec::TAXI_PATH:
      case rec::TAXI_PATH_MSFS:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          StructureType structType = STRUCT_FSX;
          if(type == rec::TAXI_PATH_P3D_V5)
            structType = STRUCT_P3DV5;
          else if(type == rec::TAXI_PATH_P3D_V4)
            structType = STRUCT_P3DV4;
          else if(type == rec::TAXI_PATH_MSFS)
            structType = STRUCT_MSFS;

          int numPaths = stream->readUShort();
          for(int i = 0; i < numPaths; i++)
          {
            TaxiPath path(stream, structType);
            taxipath::Type pathType = path.getType();

            if(pathType == atools::fs::bgl::taxipath::TAXI || pathType == atools::fs::bgl::taxipath::PATH ||
               pathType == atools::fs::bgl::taxipath::PARKING)
              taxipaths.append(path);
          }
        }
        break;

      case rec::TAXI_POINT_P3DV5:
      case rec::TAXI_POINT:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          int numPoints = stream->readUShort();
          for(int i = 0; i < numPoints; i++)
          {
            TaxiPoint taxiPoint(stream, type == rec::TAXI_POINT_P3DV5 ? STRUCT_P3DV5 : STRUCT_FSX);

#ifdef DEBUG_INFORMATION_TAXI
            if(!taxiPoint.isValid())
              qWarning() << Q_FUNC_INFO << "Invalid taxi point #" << i << ident;
#endif
            taxipoints.append(taxiPoint);
          }
        }
        break;

      case rec::TAXI_NAME:
        if(options->isIncludedNavDbObject(type::TAXIWAY))
        {
          int numNames = stream->readUShort();
          for(int i = 0; i < numNames; i++)
            taxinames.append(stream->readString(8, atools::io::LATIN1)); // First is always 0 and length always 8
        }
        break;

      // Expected records
      case rec::FENCE_BOUNDARY:
      case rec::FENCE_BLAST:
      case rec::APRON_EDGE_LIGHTS:
      case rec::AIRPORT_UNKNOWN_003B:
      case rec::MSFS_AIRPORT_LIGHT_SUPPORT:
      case rec::MSFS_UNKNOWN_00CD:
      case rec::MSFS_AIRPORT_PAINTED_LINE:
      case rec::MSFS_AIRPORT_PAINTED_HATCHED_AREA:
      case rec::MSFS_AIRPORT_TAXIWAY_SIGN:
      case rec::MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME:
      case rec::MSFS_AIRPORT_JETWAY:
      case rec::DELETE_AIRPORT_NAVIGATION:
      case rec::MSFS_AIRPORT_PROJECTED_MESH:
      case rec::MSFS_AIRPORT_GROUND_MERGING_TRANSFER:
      case rec::MSFS_AIRPORT_UNKNOWN_0058:
      case rec::MSFS_AIRPORT_UNKNOWN_0059:
      case rec::MSFS_AIRPORT_UNKNOWN_005A:
      case rec::MSFS_AIRPORT_UNKNOWN_005B:
        break;

      default:
        qWarning().noquote().nospace() << Q_FUNC_INFO << " Unexpected record type in airport record for " << ident
                                       << hex << " 0x" << r.getId()
                                       << dec << " " << airportRecordTypeStr(type) << " offset " << stream->tellg();

        if(subrecordIndex == 0)
        {
          qWarning().nospace().noquote() << Q_FUNC_INFO << " Ignoring airport. Unexpected initial record type in Airport record 0x"
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

  removeVehicleParking();

  // Update all the number fields and the bounding rectangle
  updateSummaryFields();

  if(flags & atools::fs::bgl::AIRPORT_MSFS_DUMMY)
    // Add a delete record for an MSFS dummy airport which contains only approaches and COM
    deleteAirports.append(DeleteAirport(del::APPROACHES | del::COMS));

  if(deleteAirports.size() > 1)
    qWarning() << Q_FUNC_INFO << "Found more than one delete record in" << getObjectName();

  // Print warnings for any invalid procedure legs =========================
  for(const Approach& app : qAsConst(approaches))
  {
    for(const ApproachLeg& leg: app.getLegs())
    {
      if(!leg.isValid())
        qWarning() << Q_FUNC_INFO << "Invalid approach leg in" << ident << app.getDescription();
    }

    for(const ApproachLeg& leg: app.getMissedLegs())
    {
      if(!leg.isValid())
        qWarning() << Q_FUNC_INFO << "Invalid missed approach leg in" << ident << app.getDescription();
    }

    for(const Transition& trans : app.getTransitions())
    {
      for(const ApproachLeg& leg: trans.getLegs())
      {
        if(!leg.isValid())
          qWarning() << Q_FUNC_INFO << "Invalid transition leg in" << ident << app.getDescription()
                     << trans.getDescription();
      }
    }
  }

  for(const SidStar& sidStar : qAsConst(sidsAndStars))
  {
    for(const ApproachLeg& leg: sidStar.getCommonRouteLegs())
    {
      if(!leg.isValid())
        qWarning() << Q_FUNC_INFO << "Invalid common route leg in" << ident << sidStar.getDescription();
    }

    for(const QList<atools::fs::bgl::ApproachLeg>& legs : sidStar.getEnrouteTransitions())
    {
      for(const ApproachLeg& leg: legs)
      {
        if(!leg.isValid())
          qWarning() << Q_FUNC_INFO << "Invalid enroute transition leg in" << ident << sidStar.getDescription();
      }
    }

    for(const QList<atools::fs::bgl::ApproachLeg>& legs : sidStar.getRunwayTransitionLegs())
    {
      for(const ApproachLeg& leg: legs)
      {
        if(!leg.isValid())
          qWarning() << Q_FUNC_INFO << "Invalid runway transition leg in" << ident << sidStar.getDescription();
      }
    }
  }

  // =====================================================
  // Now remove all procedures which are not valid in any way
  approaches.erase(std::remove_if(approaches.begin(), approaches.end(),
                                  [](const Approach& approach) -> bool {
          return !approach.isValid();
        }), approaches.end());

  sidsAndStars.erase(std::remove_if(sidsAndStars.begin(), sidsAndStars.end(),
                                    [](const SidStar& sidStar) -> bool {
          return !sidStar.isValid();
        }), sidsAndStars.end());
}

Airport::~Airport()
{
}

bool Airport::isMsfsPoiDummy(bool addon) const
{
  return isAirportClosed() && !addon &&
         runways.isEmpty() && taxipaths.isEmpty() && helipads.isEmpty() && parkings.isEmpty() && starts.isEmpty() &&
         // Allows empty airports in navdata updating procedures and COM
         approaches.isEmpty() && sidsAndStars.isEmpty() && coms.isEmpty() && !aprons.isEmpty();
}

bool Airport::isCurrentRecordValid()
{
  Record tempRec(opts, bs);
  rec::AirportRecordType tempType = tempRec.getId<rec::AirportRecordType>();
  bool valid = bgl::rec::airportRecordTypeValid(tempType) && tempRec.isFullyValid();
  tempRec.seekToStart();
  return valid;
}

bool Airport::isValid() const
{
  return !ident.isEmpty() && position.getPos().isValid() && !position.getPos().isNull();
}

QString Airport::getObjectName() const
{
  return Record::getObjectName() + QString("airport ident %1 region %2 name %3 position %4").
         arg(ident).arg(region).arg(name).arg(position.getPos().toString());
}

void Airport::extractMainComFrequencies(const QList<Com>& coms, int& towerFrequency, int& unicomFrequency,
                                        int& awosFrequency, int& asosFrequency, int& atisFrequency)
{
  for(const Com& com : coms)
  {
    // Use lowest frequency for default to have it deterministic
    if((com.getType() == com::TOWER || com.getType() == com::TOWER_P3D_V5) && (towerFrequency == 0 || com.getFrequency() < towerFrequency))
      towerFrequency = com.getFrequency();
    else if((com.getType() == com::UNICOM || com.getType() == com::UNICOM_P3D_V5) &&
            (unicomFrequency == 0 || com.getFrequency() < unicomFrequency))
      unicomFrequency = com.getFrequency();
    else if((com.getType() == com::AWOS || com.getType() == com::AWOS_P3D_V5) && (awosFrequency == 0 || com.getFrequency() < awosFrequency))
      awosFrequency = com.getFrequency();
    else if((com.getType() == com::ASOS || com.getType() == com::ASOS_P3D_V5) && (asosFrequency == 0 || com.getFrequency() < asosFrequency))
      asosFrequency = com.getFrequency();
    else if((com.getType() == com::ATIS || com.getType() == com::ATIS_P3D_V5) && (atisFrequency == 0 || com.getFrequency() < atisFrequency))
      atisFrequency = com.getFrequency();
  }
}

void Airport::reportFarCoordinate(const atools::geo::Pos& pos, const QString& text)
{
  if(opts->isAirportValidation())
  {
    float dist = atools::geo::meterToNm(position.getPos().distanceMeterTo(pos));
    if(dist > 10.f)
      qWarning() << Q_FUNC_INFO << "Airport" << ident << "at" << position.getPos()
                 << "has far" << text << "coordinate" << pos << "at" << dist << "NM";
  }
}

void Airport::updateSummaryFields()
{
  atools::geo::LineString points;

  const geo::Pos& pos = position.getPos();
  if(!pos.isNull() && pos.isValidRange())
    points.append(pos);

  if(!towerPosition.getPos().isNull() && towerPosition.getPos().isValidRange() && !towerPosition.getPos().isPole())
  {
    reportFarCoordinate(towerPosition.getPos(), "tower");
    points.append(towerPosition.getPos());
  }

  for(const Runway& rw : qAsConst(runways))
  {
    // Count runway types
    if(rw.getEdgeLight() != rw::NO_LIGHT)
      numLightRunway++;

    // Extend bounding rectangle for runway dimensions
    reportFarCoordinate(rw.getPosition().getPos(), "runway");
    points.append(rw.getPosition().getPos());

    reportFarCoordinate(rw.getSecondaryPosition(), "runway secondary");
    points.append(rw.getSecondaryPosition());

    reportFarCoordinate(rw.getSecondaryPosition(), "runway primary");
    points.append(rw.getPrimaryPosition());

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

    if(bgl::util::isFlagNotSet(rw.getMarkingFlags(), rw::PRIMARY_CLOSED) &&
       bgl::util::isFlagNotSet(rw.getMarkingFlags(), rw::SECONDARY_CLOSED))
      numRunwayFullOpen++;

    if(bgl::util::isFlagSet(rw.getMarkingFlags(), rw::PRIMARY_CLOSED) &&
       bgl::util::isFlagSet(rw.getMarkingFlags(), rw::SECONDARY_CLOSED))
      numRunwayFullClosed++;

    if(bgl::util::isFlagSet(rw.getMarkingFlags(), rw::PRIMARY_CLOSED))
      numRunwayEndClosed++;

    if(bgl::util::isFlagSet(rw.getMarkingFlags(), rw::SECONDARY_CLOSED))
      numRunwayEndClosed++;
  }

  // Completely rely on closed flag for MSFS - check runways for other simulators
  if(!msfs)
  {
    // If all runways are closed the airport is closed ...
    airportClosed = !runways.isEmpty() && numRunwayFullClosed == runways.size();

    // ... except if there are open helipads
    for(const Helipad& pad : qAsConst(helipads))
    {
      if(!pad.isClosed())
      {
        airportClosed = false;
        break;
      }
    }
  }

  for(const Parking& p : qAsConst(parkings))
  {
    reportFarCoordinate(p.getPosition().getPos(), "parking");
    points.append(p.getPosition().getPos());

    // Assign fuel from parking if not set in flags
    // https://devsupport.flightsimulator.com/questions/10232/su10-u3-removed-fuel-flags-from-bgl-files.html
    if(p.isFuel() && msfs && fuelFlags == ap::NO_FUEL_FLAGS)
      fuelFlags = atools::fs::bgl::ap::MSFS_DEFAULT_FUEL;

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

  for(const Apron& a : qAsConst(aprons))
  {
    // reportFarCoordinate(s.getPosition().getPos(), "start"); // Too CPU intense
    for(const BglPosition& p : a.getVertices())
    {
      reportFarCoordinate(p.getPos(), "apron");
      points.append(p.getPos());
    }
  }

  for(const Apron2& a : qAsConst(aprons2))
  {
    // reportFarCoordinate(s.getPosition().getPos(), "start"); // Too CPU intense
    for(const BglPosition& p : a.getVertices())
    {
      reportFarCoordinate(p.getPos(), "apron2");
      points.append(p.getPos());
    }
  }

  for(const Start& s : qAsConst(starts))
  {
    reportFarCoordinate(s.getPosition().getPos(), "start");
    points.append(s.getPosition().getPos());
  }

  for(const Helipad& h : qAsConst(helipads))
  {
    reportFarCoordinate(h.getPosition().getPos(), "helipad");
    points.append(h.getPosition().getPos());
  }

  for(const TaxiPath& p : qAsConst(taxipaths))
  {
    reportFarCoordinate(p.getStartPoint().getPosition().getPos(), "taxipath start");
    points.append(p.getStartPoint().getPosition().getPos());

    reportFarCoordinate(p.getEndPoint().getPosition().getPos(), "taxipath end");
    points.append(p.getEndPoint().getPosition().getPos());
  }

  updateHelipads();

  boundingRect = atools::geo::bounding(points);
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
        qWarning().nospace().noquote() << Q_FUNC_INFO << "Parking for jetway " << jw << " already set" << dec
                                       << " for parking " << parkings.at(index)
                                       << " for ident " << ident;
      else
        parkings[index].jetway = true;
    }
    else
      qWarning().nospace().noquote() << Q_FUNC_INFO << "Parking for jetway " << jw << " not found" << dec
                                     << " for ident " << ident;
  }
}

void Airport::updateHelipads()
{
  for(Helipad& helipad : helipads)
  {
    int startIdx = 1;
    for(const Start& start : qAsConst(starts))
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
  parkings.erase(std::remove_if(parkings.begin(), parkings.end(), [](const Parking& p) -> bool {
          return p.getType() == atools::fs::bgl::ap::VEHICLES;
        }), parkings.end());
}

void Airport::updateTaxiPaths(const QList<TaxiPoint>& taxipoints, const QStringList& taxinames)
{
  using atools::inRange;

  // Assign names ========================================================
  for(TaxiPath& taxiPath : taxipaths)
  {
    switch(taxiPath.type)
    {
      case atools::fs::bgl::taxipath::UNKNOWN:
      case atools::fs::bgl::taxipath::CLOSED:
      case atools::fs::bgl::taxipath::VEHICLE:
      case atools::fs::bgl::taxipath::RUNWAY:
        break;

      case atools::fs::bgl::taxipath::PATH:
      case atools::fs::bgl::taxipath::TAXI:
      case atools::fs::bgl::taxipath::PARKING:
        if(inRange(taxinames, taxiPath.nameIndex))
          taxiPath.taxiName = taxinames.at(taxiPath.nameIndex);
        else
          qWarning() << Q_FUNC_INFO << "Taxiway name index out of bounds in" << ident
                     << "path" << taxiPath << "taxinames.size()" << taxinames.size();
        break;
    }
  }

  // Assign start and end positions ========================================================
  for(TaxiPath& taxiPath : taxipaths)
  {
    switch(taxiPath.type)
    {
      case atools::fs::bgl::taxipath::UNKNOWN:
      case atools::fs::bgl::taxipath::CLOSED:
      case atools::fs::bgl::taxipath::VEHICLE:
      case atools::fs::bgl::taxipath::RUNWAY:
        break;

      case atools::fs::bgl::taxipath::PATH:
      case atools::fs::bgl::taxipath::TAXI:
        if(inRange(taxipoints, taxiPath.startIndex) && inRange(taxipoints, taxiPath.endIndex))
        {
          taxiPath.startPos = taxipoints.at(taxiPath.startIndex);
          taxiPath.endPos = taxipoints.at(taxiPath.endIndex);
        }
        break;

      case atools::fs::bgl::taxipath::PARKING:
        if(inRange(taxipoints, taxiPath.startIndex) && inRange(parkings, taxiPath.endIndex))
        {
          taxiPath.startPos = taxipoints.at(taxiPath.startIndex);
          taxiPath.endPos = TaxiPoint(parkings.at(taxiPath.endIndex));
        }
        break;
    }
  }

  for(int i = 0; i < taxipaths.mid(0, 50).size(); i++)
  {
    const TaxiPath& path = taxipaths.at(i);
    if(!path.isValid())
      qWarning() << Q_FUNC_INFO << "Invalid taxi path in" << ident << "#" << i << "of" << taxipaths.size() << "path" << path;
  }

  // Remove all paths that remain invalid because of wrong indexes
  int num = taxipaths.size();
  taxipaths.erase(std::remove_if(taxipaths.begin(), taxipaths.end(), [](const TaxiPath& path) -> bool {
          return !path.isValid();
        }), taxipaths.end());

  if(num != taxipaths.size())
    qWarning() << Q_FUNC_INFO << "Removed" << (num - taxipaths.size()) << "invalid taxipaths showing max 50 invalid";
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
  out << record.approaches;
  out << record.sidsAndStars;
  out << record.parkings;
  out << record.deleteAirports;
  out << record.helipads;
  out << record.starts;
  out << record.taxipaths;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
