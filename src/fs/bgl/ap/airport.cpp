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

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Airport::Airport(BinaryStream *bs)
  : Record(bs)
{
  numRunways = bs->readByte();
  numComs = bs->readByte();
  numStarts = bs->readByte();
  numApproaches = bs->readByte();
  numAprons = bs->readByte();
  deleteRecord = (numAprons & 0x80) == 0x80;
  numHelipads = bs->readByte();
  position = BglPosition(bs, 1000.f);
  towerPosition = BglPosition(bs, 1000.f);
  magVar = bs->readFloat();
  ident = converter::intToIcao(bs->readUInt());
  region = converter::intToIcao(bs->readUInt());
  fuelFlags = bs->readUInt();

  bs->skip(4);

  while(bs->tellg() < startOffset + size)
  {
    Record r(bs);
    rec::AirportRecordType t = r.getId<rec::AirportRecordType>();

    switch(t)
    {
      case rec::NAME:
        name = bs->readString(r.getSize() - 6);
        break;
      case rec::RUNWAY:
        r.seekToStart();
        runways.push_back(Runway(bs, ident));
        break;
      case rec::COM:
        r.seekToStart();
        coms.push_back(Com(bs));
        break;
      case rec::TAXI_PARKING:
        handleParking();
        break;
      case rec::APPROACH:
        r.seekToStart();
        approaches.push_back(Approach(bs));
        break;
      case rec::AIRPORT_WAYPOINT:
        r.seekToStart();
        waypoints.push_back(Waypoint(bs));
        break;
      case rec::DELETE_AIRPORT:
        r.seekToStart();
        deleteAirports.push_back(DeleteAirport(bs));
        break;

      case rec::APRON_FIRST:
        apron = true;
        break;
      case rec::JETWAY:
        jetway = true;
        break;
      case rec::FENCE_BOUNDARY:
        boundaryFence = true;
        break;
      case rec::TOWER_OBJ:
        towerObj = true;
        break;
      case rec::TAXI_PATH:
        taxiway = true;
        break;

      case rec::APRON_SECOND:
      case rec::APRON_EDGE_LIGHTS:
      case rec::HELIPAD:
      case rec::START:
      case rec::TAXI_POINT:
      case rec::TAXI_NAME:
      case rec::FENCE_BLAST:
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
  out << "]";

  return out;
}

Airport::~Airport()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
