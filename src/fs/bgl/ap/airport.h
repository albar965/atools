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

#ifndef BGL_AIRPORT_H_
#define BGL_AIRPORT_H_

#include "fs/bgl/record.h"
#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/ap/approach.h"
#include "fs/bgl/ap/parking.h"
#include "del/deleteairport.h"
#include "fs/bgl//bglposition.h"
#include "fs/bgl/ap/rw/runway.h"

#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum FuelAvailability
{
  NO_FUEL = 0, UNKNOWN_FUEL = 1, PRIOR_REQUEST = 2, YES = 3
};

enum FuelFlags
{
  OCTANE_73 = 0x00000003, // 0-1 :
  OCTANE_87 = 0x0000000c, // Bits 2-3 :
  OCTANE_100 = 0x00000030, // Bits 4-5 :
  OCTANE_130 = 0x000000c0, // Bits 6-7 :
  OCTANE_145 = 0x00000300, // Bits 8-9 :
  MOGAS = 0x00000c00, // Bits 10-11 :
  JET = 0x00003000, // Bits 12-13 :
  JETA = 0x0000c000, // Bits 14-15 :
  JETA1 = 0x00030000, // Bits 16-17 :
  JETAP = 0x000c0000, // Bits 18-19 :
  JETB = 0x00300000, // Bits 20-21 :
  JET4 = 0x00c00000, // Bits 22-23 :
  JET5 = 0x03000000, // Bits 24-22 : // TODO error in wiki
  AVGAS = 0x40000000, // Bit 30 :
  JET_FUEL = 0x40000000 // Bit 31 :
};

} // namespace ap

class Airport :
  public atools::fs::bgl::Record
{
public:
  Airport(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Airport();

  const QList<atools::fs::bgl::Approach>& getApproaches() const
  {
    return approaches;
  }

  bool hasApron() const
  {
    return apron;
  }

  bool hasBoundaryFence() const
  {
    return boundaryFence;
  }

  const QList<atools::fs::bgl::Com>& getComs() const
  {
    return coms;
  }

  const QList<atools::fs::bgl::DeleteAirport>& getDeleteAirports() const
  {
    return deleteAirports;
  }

  unsigned int getFuelFlags() const
  {
    return fuelFlags;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  bool hasJetway() const
  {
    return jetway;
  }

  float getMagVar() const
  {
    return magVar;
  }

  const QString& getName() const
  {
    return name;
  }

  int getNumHelipads() const
  {
    return numHelipads;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  const QString& getRegion() const
  {
    return region;
  }

  const QList<atools::fs::bgl::Runway>& getRunways() const
  {
    return runways;
  }

  bool hasTaxiway() const
  {
    return taxiway;
  }

  bool hasTowerObj() const
  {
    return towerObj;
  }

  const atools::fs::bgl::BglPosition& getTowerPosition() const
  {
    return towerPosition;
  }

  const QList<atools::fs::bgl::Waypoint>& getWaypoints() const
  {
    return waypoints;
  }

  const QList<atools::fs::bgl::Parking>& getParkings() const
  {
    return parkings;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Airport& record);
  void handleParking();

  int numRunways, numComs, numStarts, numApproaches, numAprons;

  bool deleteRecord;
  int numHelipads;
  atools::fs::bgl::BglPosition position, towerPosition;
  float magVar;
  QString ident;
  QString region;

  unsigned int fuelFlags;
  QString name;
  bool apron, jetway, boundaryFence, towerObj, taxiway;

  QList<atools::fs::bgl::Runway> runways;
  QList<atools::fs::bgl::Parking> parkings;
  QList<atools::fs::bgl::Com> coms;
  QList<atools::fs::bgl::Approach> approaches;
  QList<atools::fs::bgl::Waypoint> waypoints;
  QList<atools::fs::bgl::DeleteAirport> deleteAirports;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORT_H_ */
