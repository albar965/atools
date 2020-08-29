/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_AIRPORT_H
#define ATOOLS_BGL_AIRPORT_H

#include "fs/bgl/record.h"
#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/ap/approach.h"
#include "fs/bgl/ap/apron.h"
#include "fs/bgl/ap/apron2.h"
#include "fs/bgl/ap/parking.h"
#include "del/deleteairport.h"
#include "fs/bgl//bglposition.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/helipad.h"
#include "fs/bgl/ap/start.h"
#include "fs/bgl/ap/taxipath.h"
#include "geo/rect.h"
#include "fs/bgl/bglfile.h"

#include <QList>
#include <QHash>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {
class Jetway;

namespace ap {

enum FuelAvailability
{
  NO_FUEL = 0,
  UNKNOWN_FUEL = 1,
  PRIOR_REQUEST = 2,
  YES = 3
};

enum FuelFlags
{
  NO_FUEL_FLAGS = 0,
  OCTANE_73 = 0x00000003, // 0-1
  OCTANE_87 = 0x0000000c, // Bits 2-3
  OCTANE_100 = 0x00000030, // Bits 4-5
  OCTANE_130 = 0x000000c0, // Bits 6-7
  OCTANE_145 = 0x00000300, // Bits 8-9
  MOGAS = 0x00000c00, // Bits 10-11
  JET = 0x00003000, // Bits 12-13
  JETA = 0x0000c000, // Bits 14-15
  JETA1 = 0x00030000, // Bits 16-17
  JETAP = 0x000c0000, // Bits 18-19
  JETB = 0x00300000, // Bits 20-21
  JET4 = 0x00c00000, // Bits 22-23
  JET5 = 0x03000000, // Bits 24-22
  AVGAS = 0x40000000, // Bit 30
  JET_FUEL = 0x80000000 // Bit 31
};

} // namespace ap

struct ParkingKey;

/*
 *  Airport aggreating all subrecords like runway, apron, taxiways and more
 */
class Airport :
  public atools::fs::bgl::Record
{
public:
  Airport(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs,
          atools::fs::bgl::flags::CreateFlags flags);
  virtual ~Airport();

  const QList<atools::fs::bgl::Approach>& getApproaches() const
  {
    return approaches;
  }

  /*
   * @return all communication frequencies for this airport
   */
  const QList<atools::fs::bgl::Com>& getComs() const
  {
    return coms;
  }

  /*
   * @return Delete airport record used to remove default/stock airports partially or full
   */
  const QList<atools::fs::bgl::DeleteAirport>& getDeleteAirports() const
  {
    return deleteAirports;
  }

  atools::fs::bgl::ap::FuelFlags getFuelFlags() const
  {
    return fuelFlags;
  }

  /*
   * @return airport ICAO ident
   */
  const QString& getIdent() const
  {
    return ident;
  }

  /*
   * @return Magnetic variance for airport. < 0 for West and > 0 for East
   */
  float getMagVar() const
  {
    return magVar;
  }

  /*
   * Name of the airport. City, state and country can be taken from the name list record.
   */
  const QString& getName() const
  {
    return name;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Two letter ICAO region ident for the airport (currently always null)
   */
  const QString& getRegion() const
  {
    return region;
  }

  const QList<atools::fs::bgl::Runway>& getRunways() const
  {
    return runways;
  }

  bool hasTowerObj() const
  {
    return towerObj;
  }

  /*
   *  @return tower object position of tower viewpoint position
   */
  const atools::fs::bgl::BglPosition& getTowerPosition() const
  {
    return towerPosition;
  }

  /*
   * @return waypoint subrecords that are attached to this airport
   */
  const QList<atools::fs::bgl::Waypoint>& getWaypoints() const
  {
    return waypoints;
  }

  const QList<atools::fs::bgl::Parking>& getParkings() const
  {
    return parkings;
  }

  const QList<atools::fs::bgl::Helipad>& getHelipads() const
  {
    return helipads;
  }

  const QList<atools::fs::bgl::Start>& getStarts() const
  {
    return starts;
  }

  const QList<atools::fs::bgl::Apron>& getAprons() const
  {
    return aprons;
  }

  const QList<atools::fs::bgl::Apron2>& getAprons2() const
  {
    return aprons2;
  }

  const QList<atools::fs::bgl::TaxiPath>& getTaxiPaths() const
  {
    return taxipaths;
  }

  /*
   * @return true if all runway ends have a yellow X for closed runway
   */
  bool isAirportClosed() const
  {
    return airportClosed;
  }

  /*
   * @return Bounding rectangle including taxiways, parking, tower position, runway extensions and more
   */
  const atools::geo::Rect& getBoundingRect() const
  {
    return boundingRect;
  }

  /*
   * @return number of runway ends that approach light system
   */
  int getNumRunwayEndApproachLight() const
  {
    return numRunwayEndApproachLight;
  }

  /*
   * @return number of runway ends that have an ILS
   */
  int getNumRunwayEndIls() const
  {
    return numRunwayEndIls;
  }

  int getNumRunwayEndClosed() const
  {
    return numRunwayEndClosed;
  }

  /*
   * @return number of runways that have at least edge lights
   */
  int getNumLightRunway() const
  {
    return numLightRunway;
  }

  /*
   * @return number of runways that have approach indicators
   */
  int getNumRunwayEndVasi() const
  {
    return numRunwayEndVasi;
  }

  int getNumJetway() const
  {
    return numJetway;
  }

  /*
   * @return number of parking that are type GA ramg
   */
  int getNumParkingGaRamp() const
  {
    return numParkingGaRamp;
  }

  /*
   * @return number of parking that are of type gate
   */
  int getNumParkingGate() const
  {
    return numParkingGate;
  }

  /*
   * @return Length of the longest runway in meter
   */
  float getLongestRunwayLength() const
  {
    return longestRunwayLength;
  }

  /*
   * @return width of the longest runway in meter
   */
  float getLongestRunwayWidth() const
  {
    return longestRunwayWidth;
  }

  /*
   * @return heading of the longest runway in degree true
   */float getLongestRunwayHeading() const
  {
    return longestRunwayHeading;
  }

  atools::fs::bgl::Surface getLongestRunwaySurface() const
  {
    return longestRunwaySurface;
  }

  atools::fs::bgl::ap::ParkingType getLargestParkingGaRamp() const
  {
    return largestParkingGaRamp;
  }

  atools::fs::bgl::ap::ParkingType getLargestParkingGate() const
  {
    return largestParkingGate;
  }

  /*
   * @return true if the airport is military. A military airport is recognized by name pattern
   */
  bool isMilitary() const
  {
    return military;
  }

  int getNumParkingCargo() const
  {
    return numParkingCargo;
  }

  int getNumParkingMilitaryCargo() const
  {
    return numParkingMilitaryCargo;
  }

  int getNumParkingMilitaryCombat() const
  {
    return numParkingMilitaryCombat;
  }

  /*
   *  Check if this is a dummy airport that comes with some elevation adjustments
   * @return true if there are no runways, no parking, etc.
   */
  bool isEmpty() const;

  static bool isNameMilitary(const QString& airportName);

  int calculateRating(bool isAddon) const;

  virtual bool isValid() const override;
  virtual QString getObjectName() const override;

  /* An empty airport containing only procedures and COM records */
  bool isMsfsDummyAirport() const
  {
    return msfsDummyAirport;
  }

  void updateRunwaySummaryFields();

  /* Extract main frequencies for overview. All frequency in MHz * 1000. */
  static void extractMainComFrequencies(const QList<atools::fs::bgl::Com>& coms, int& towerFrequency,
                                        int& unicomFrequency, int& awosFrequency, int& asosFrequency,
                                        int& atisFrequency);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Airport& record);

  void updateTaxiPaths(const QList<TaxiPoint>& taxipoints, const QStringList& taxinames);
  void updateParking(const QList<atools::fs::bgl::Jetway>& jetways,
                     const QHash<atools::fs::bgl::ParkingKey, int>& parkingNumberIndex);
  void updateSummaryFields();
  void removeVehicleParking();
  void updateHelipads();
  bool isCurrentRecordValid();

  /* Minimum runway length - if smaller it is considered a dummy runway that was just added for ATC/traffic */
  static const int MIN_RUNWAY_LENGTH_METER = 10;

  /* remove all runways that are far away from the airport center postition */
  static const int MAX_RUNWAY_DISTANCE_METER = 50000;

  atools::fs::bgl::BglPosition position, towerPosition;
  atools::geo::Rect boundingRect;
  float magVar;
  QString ident, name, region;

  atools::fs::bgl::ap::FuelFlags fuelFlags;
  bool towerObj = false, airportClosed = false, military = false,
       msfsStar = false /* Will result in five stars rating*/,
       msfsDummyAirport = false /* From navdata update. Empty airport with COM and procedures */;

  int numRunwayEndApproachLight = 0, numRunwayEndIls = 0, numRunwayEndClosed = 0, numLightRunway = 0,
      numRunwayEndVasi = 0, numJetway = 0, numParkingGaRamp = 0, numParkingGate = 0,
      numParkingCargo = 0, numParkingMilitaryCargo = 0, numParkingMilitaryCombat = 0;

  float longestRunwayLength = 0.f, longestRunwayWidth = 0.f, longestRunwayHeading = 0.f;

  atools::fs::bgl::Surface longestRunwaySurface = atools::fs::bgl::UNKNOWN;
  atools::fs::bgl::ap::ParkingType largestParkingGaRamp = atools::fs::bgl::ap::UNKNOWN,
                                   largestParkingGate = atools::fs::bgl::ap::UNKNOWN;

  QList<atools::fs::bgl::Runway> runways;
  QList<atools::fs::bgl::Parking> parkings;
  QList<atools::fs::bgl::Com> coms;
  QList<atools::fs::bgl::Helipad> helipads;
  QList<atools::fs::bgl::Start> starts;
  QList<atools::fs::bgl::Approach> approaches;
  QList<atools::fs::bgl::Waypoint> waypoints;
  QList<atools::fs::bgl::DeleteAirport> deleteAirports;
  QList<atools::fs::bgl::Apron> aprons;
  QList<atools::fs::bgl::Apron2> aprons2;
  QList<atools::fs::bgl::TaxiPath> taxipaths;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORT_H
