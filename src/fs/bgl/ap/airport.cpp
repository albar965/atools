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

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Airport::Airport(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  numRunways = bs->readByte();
  numComs = bs->readByte();
  bs->readByte(); // numStarts
  numApproaches = bs->readByte();
  numAprons = bs->readByte();
  deleteRecord = (numAprons & 0x80) == 0x80;
  bs->readByte(); // numHelipads
  position = BglPosition(bs, 1000.f);
  towerPosition = BglPosition(bs, 1000.f);
  magVar = bs->readFloat();
  ident = converter::intToIcao(bs->readUInt());
  region = converter::intToIcao(bs->readUInt());
  fuelFlags = bs->readUInt();

  bs->skip(4);

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
          handleParking();
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
        // TODO read apron data
        apron = true;
        break;
      case rec::JETWAY:
        // TODO read jetway data
        jetway = true;
        break;
      case rec::FENCE_BOUNDARY:
        // TODO read boundary fence data
        boundaryFence = true;
        break;
      case rec::TOWER_OBJ:
        // TODO read tower object data
        towerObj = true;
        break;
      case rec::TAXI_PATH:
        // TODO read taxiway data
        taxiway = true;
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

      case rec::APRON_SECOND:
      case rec::APRON_EDGE_LIGHTS:
      // TODO read apron lights data
      case rec::TAXI_POINT:
      case rec::TAXI_NAME:
      case rec::FENCE_BLAST:
      // TODO read blast fence data
      case rec::UNKNOWN_REC:
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in Airport record 0x" << hex << t << dec
                                       << " for ident " << ident;
    }
    r.seekToEnd();
  }
}

void Airport::handleParking()
{
  int numParkings = bs->readShort();
  for(int i = 0; i < numParkings; i++)
    parkings.push_back(Parking(bs));
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
  out << record.approaches;
  out << record.parkings;
  out << record.deleteAirports;
  out << record.helipads;
  out << record.starts;
  out << "]";

  return out;
}

Airport::~Airport()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
