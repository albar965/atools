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

#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include "fs/pln/flightplanentry.h"

class QXmlStreamReader;

namespace atools {

namespace fs {
namespace pln {

enum FlightplanType
{
  UNKNOWN_TYPE,
  IFR,
  VFR
};

enum RouteType
{
  UNKNOWN_ROUTE,
  LOW_ALT,
  HIGH_ALT,
  VOR,
  DIRECT
};

class Flightplan
{
public:
  Flightplan();
  ~Flightplan();

  void load(const QString& file);
  void save(const QString& file);

  int getDepartureAirportId();
  int getDepartureParkingId();
  int getDestinationAirportId();

  void setDeparture(int airportId, int parkingId);
  void setDestionation(int airportId);

  QList<atools::fs::pln::FlightplanEntry>& getEntries()
  {
    return entries;
  }

  atools::fs::pln::FlightplanType getFlightplanType() const
  {
    return flightplanType;
  }

  void setFlightplanType(const atools::fs::pln::FlightplanType& value)
  {
    flightplanType = value;
  }

  atools::fs::pln::RouteType getRouteType() const
  {
    return routeType;
  }

  void setRouteType(const atools::fs::pln::RouteType& value)
  {
    routeType = value;
  }

  int getCruisingAlt() const
  {
    return cruisingAlt;
  }

  void setCruisingAlt(int value)
  {
    cruisingAlt = value;
  }

  QString getDescription() const
  {
    return description;
  }

  void setDescription(const QString& value)
  {
    description = value;
  }

  QString getDepartureIdent() const
  {
    return departureIdent;
  }

  QString getDestinationIdent() const
  {
    return destinationIdent;
  }

  QString getDepartureParkingName() const
  {
    return departureParkingName;
  }

  QString getDepartureAiportName() const
  {
    return departureAiportName;
  }

  QString getDestinationAiportName() const
  {
    return destinationAiportName;
  }

  void setDestinationAiportName(const QString& value)
  {
    destinationAiportName = value;
  }

private:
  static QString flightplanTypeToString(atools::fs::pln::FlightplanType type);
  static atools::fs::pln::FlightplanType stringFlightplanType(const QString& str);
  static QString routeTypeToString(atools::fs::pln::RouteType type);
  static atools::fs::pln::RouteType stringToRouteType(const QString& str);

  void readUntilElement(QXmlStreamReader& reader, const QString& name);

  atools::fs::pln::FlightplanType flightplanType;
  atools::fs::pln::RouteType routeType;

  QList<atools::fs::pln::FlightplanEntry> entries;

  int cruisingAlt, appVersionMajor, appVersionBuild;
  QString filename, title, departureIdent, destinationIdent, description, departureParkingName,
          departureAiportName, destinationAiportName;
  atools::geo::Pos departurePos, destinationPos;

  void readAppVersion(QXmlStreamReader& reader);
  void readWaypoint(QXmlStreamReader& reader);

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // FLIGHTPLAN_H
