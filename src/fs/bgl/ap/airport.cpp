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

#include "fs/bgl/ap/airport.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/bglreaderoptions.h"
#include "fs/bgl/ap/jetway.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/ap/taxipoint.h"

#include <QHash>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Airport::Airport(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  int numRunways = bs->readUByte();
  Q_UNUSED(numRunways);
  int numComs = bs->readUByte();
  Q_UNUSED(numComs);
  bs->readUByte(); // numStarts
  int numApproaches = bs->readUByte();
  Q_UNUSED(numApproaches);
  int numAprons = bs->readUByte();
  int numDeleteRecords = (numAprons & 0x80) == 0x80;
  Q_UNUSED(numDeleteRecords);
  bs->readUByte(); // numHelipads
  position = BglPosition(bs, true, 1000.f);
  towerPosition = BglPosition(bs, true, 1000.f);
  magVar = bs->readFloat();
  ident = converter::intToIcao(bs->readUInt());
  region = converter::intToIcao(bs->readUInt());
  fuelFlags = bs->readUInt();

  bs->skip(4);

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
        name = bs->readString(r.getSize() - 6);
        break;
      case rec::RUNWAY:
        if(options->includeBglObject(type::RUNWAY))
        {
          r.seekToStart();
          runways.push_back(Runway(options, bs, ident));
        }
        break;
      case rec::COM:
        if(options->includeBglObject(type::COM))
        {
          r.seekToStart();
          coms.push_back(Com(options, bs));
        }
        break;
      case rec::TAXI_PARKING:
        if(options->includeBglObject(type::PARKING))
        {
          int numParkings = bs->readUShort();
          for(int i = 0; i < numParkings; i++)
          {
            Parking p(bs);
            parkingNumberIndex.insert(p.getNumber(), parkings.size());
            parkings.push_back(p);
          }
        }
        break;
      case rec::APPROACH:
        if(options->includeBglObject(type::APPROACH))
        {
          r.seekToStart();
          approaches.push_back(Approach(options, bs));
        }
        break;
      case rec::AIRPORT_WAYPOINT:
        if(options->includeBglObject(type::WAYPOINT))
        {
          r.seekToStart();
          waypoints.push_back(Waypoint(options, bs));
        }
        break;
      case rec::DELETE_AIRPORT:
        r.seekToStart();
        deleteAirports.push_back(DeleteAirport(options, bs));
        break;

      case rec::APRON_FIRST:
        if(options->includeBglObject(type::APRON))
        {
          r.seekToStart();
          aprons.push_back(Apron(options, bs));
        }
        break;
      case rec::APRON_SECOND:
        if(options->includeBglObject(type::APRON))
        {
          r.seekToStart();
          aprons2.push_back(Apron2(options, bs));
        }
        break;
      case rec::APRON_EDGE_LIGHTS:
        if(options->includeBglObject(type::APRON))
        {
          r.seekToStart();
          apronLights.push_back(ApronLight(options, bs));
        }
        break;
      case rec::HELIPAD:
        if(options->includeBglObject(type::HELIPAD))
        {
          r.seekToStart();
          helipads.push_back(Helipad(options, bs));
        }
        break;
      case rec::START:
        if(options->includeBglObject(type::START))
        {
          r.seekToStart();
          starts.push_back(Start(options, bs));
        }
        break;
      case rec::JETWAY:
        r.seekToStart();
        jetways.push_back(Jetway(options, bs));
        break;
      case rec::FENCE_BOUNDARY:
      case rec::FENCE_BLAST:
        if(options->includeBglObject(type::FENCE))
        {
          r.seekToStart();
          fences.push_back(Fence(options, bs));
        }
        break;
      case rec::TOWER_OBJ:
        towerObj = true;
        break;
      case rec::TAXI_PATH:
        if(options->includeBglObject(type::TAXIWAY))
        {
          int numPaths = bs->readUShort();
          for(int i = 0; i < numPaths; i++)
            taxipaths.push_back(TaxiPath(bs));
        }
        break;
      case rec::TAXI_POINT:
        if(options->includeBglObject(type::TAXIWAY))
        {
          int numPoints = bs->readUShort();
          for(int i = 0; i < numPoints; i++)
            taxipoints.push_back(TaxiPoint(bs));
        }
        break;
      case rec::TAXI_NAME:
        if(options->includeBglObject(type::TAXIWAY))
        {
          int numNames = bs->readUShort();
          for(int i = 0; i < numNames; i++)
            taxinames.push_back(bs->readString(8));  // TODO fix wiki - first is always 0 and length always 8
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

  for(TaxiPath& t : taxipaths)
  {
    switch(t.type)
    {
      case atools::fs::bgl::taxi::UNKNOWN_PATH_TYPE :
        break;
      case atools::fs::bgl::taxi::PATH:
      case atools::fs::bgl::taxi::CLOSED:
      case atools::fs::bgl::taxi::TAXI:
      case atools::fs::bgl::taxi::VEHICLE:
        t.taxiName = taxinames.at(t.runwayNumTaxiName);
      case atools::fs::bgl::taxi::RUNWAY:
        t.start = taxipoints.at(t.startPoint);
        t.end = taxipoints.at(t.endPoint);
        break;
      case atools::fs::bgl::taxi::PARKING:
        t.taxiName = taxinames.at(t.runwayNumTaxiName);
        t.start = taxipoints.at(t.startPoint);
        t.end = TaxiPoint(parkings.at(t.endPoint));
        break;
    }
  }

  for(const Jetway& jw : jetways)
  {
    QHash<int, int>::const_iterator iter = parkingNumberIndex.find(jw.getParkingIndex());
    if(iter != parkingNumberIndex.end())
      parkings[iter.value()].setHasJetway(true);
    else
      qWarning().nospace().noquote() << "Parking for jetway " << jw << " not found" << dec
                                     << " for ident " << ident;
  }

  // TODO create warnings for this
  // Q_ASSERT(runways.size() == numRunways);
  // Q_ASSERT(approaches.size() == numApproaches);
  // Q_ASSERT(deleteAirports.size() == numDeleteRecords);
  // Q_ASSERT(coms.size() == numComs);
}

QDebug operator<<(QDebug out, const Airport& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Airport[ICAO " << record.ident
  << ", name " << record.name
  << ", region " << record.region
  << ", " << record.position
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

Airport::~Airport()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
