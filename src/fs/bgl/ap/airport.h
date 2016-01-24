/*
 * AirportRecord.h
 *
 *  Created on: 20.04.2015
 *      Author: alex
 */

#ifndef BGL_AIRPORT_H_
#define BGL_AIRPORT_H_

#include "../record.h"
#include "../nav/waypoint.h"
#include "approach.h"
#include "parking.h"
#include "del/deleteairport.h"
#include "../bglposition.h"
#include "rw/runway.h"

#include <QString>
#include <QList>
#include "com.h"

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
  public Record
{
public:
  Airport(atools::io::BinaryStream *bs);
  virtual ~Airport();

  const QList<Approach>& getApproaches() const
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

  const QList<Com>& getComs() const
  {
    return coms;
  }

  const QList<DeleteAirport>& getDeleteAirports() const
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

  const BglPosition& getPosition() const
  {
    return position;
  }

  const QString& getRegion() const
  {
    return region;
  }

  const QList<Runway>& getRunways() const
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

  const BglPosition& getTowerPosition() const
  {
    return towerPosition;
  }

  const QList<Waypoint>& getWaypoints() const
  {
    return waypoints;
  }

  const QList<Parking>& getParkings() const
  {
    return parkings;
  }

private:
  friend QDebug operator<<(QDebug out, const Airport& record);
  void handleParking();

  int numRunways, numComs, numStarts, numApproaches, numAprons;

  bool deleteRecord;
  int numHelipads;
  BglPosition position, towerPosition;
  float magVar;
  QString ident;
  QString region;

  unsigned int fuelFlags;
  QString name;
  bool apron, jetway, boundaryFence, towerObj, taxiway;

  QList<Runway> runways;
  QList<Parking> parkings;
  QList<Com> coms;
  QList<Approach> approaches;
  QList<Waypoint> waypoints;
  QList<DeleteAirport> deleteAirports;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORT_H_ */
