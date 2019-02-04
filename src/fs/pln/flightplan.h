/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FLIGHTPLAN_H
#define ATOOLS_FLIGHTPLAN_H

#include "fs/pln/flightplanentry.h"
#include "fs/pln/flightplanconstants.h"

#include <QApplication>

class QXmlStreamReader;

namespace atools {
namespace geo {
class LineString;
}
namespace fs {
namespace pln {

class FlightplanIO;

/*
 * A class to load, modify and save FSX (and all other compatible simulators) flight plans.
 */
class Flightplan
{
  Q_DECLARE_TR_FUNCTIONS(Flightplan)

public:
  Flightplan();
  Flightplan(const atools::fs::pln::Flightplan& other);
  ~Flightplan();

  atools::fs::pln::Flightplan& operator=(const atools::fs::pln::Flightplan& other);

  /*
   * @return Get all flight plan entries/waypoints. These include start and destination.
   */
  QList<atools::fs::pln::FlightplanEntry>& getEntries()
  {
    return entries;
  }

  /* Clear out all entries with no save = true */
  void removeNoSaveEntries();

  /*
   * @return Get all flight plan entries/waypoints. These include start and destination.
   */
  const QList<atools::fs::pln::FlightplanEntry>& getEntries() const
  {
    return entries;
  }

  float getDistanceNm() const;

  const atools::fs::pln::FlightplanEntry& at(int index) const
  {
    return entries.at(index);
  }

  atools::fs::pln::FlightplanEntry& operator[](int index)
  {
    return entries[index];
  }

  atools::fs::pln::FlightplanType getFlightplanType() const
  {
    return flightplanType;
  }

  void setFlightplanType(atools::fs::pln::FlightplanType value)
  {
    flightplanType = value;
  }

  atools::fs::pln::RouteType getRouteType() const
  {
    return routeType;
  }

  void setRouteType(atools::fs::pln::RouteType value)
  {
    routeType = value;
  }

  /*
   * @return cruise altitude in feet
   */
  int getCruisingAltitude() const
  {
    return cruisingAlt;
  }

  void setCruisingAltitude(int value)
  {
    cruisingAlt = value;
  }

  /*
   * @return "Descr" element of the file like "LOAG, LSZG"
   */
  const QString& getDescription() const
  {
    return description;
  }

  void setDescription(const QString& value)
  {
    description = value;
  }

  /*
   * @return departure ICAO code
   */
  const QString& getDepartureIdent() const
  {
    return departureIdent;
  }

  /*
   * @return destination ICAO code
   */
  const QString& getDestinationIdent() const
  {
    return destinationIdent;
  }

  /*
   * @return departure parking name like "PARKING 2"
   */
  const QString& getDepartureParkingName() const
  {
    return departureParkingName;
  }

  const QString& getDepartureAiportName() const
  {
    return departureAiportName;
  }

  const QString& getDestinationAiportName() const
  {
    return destinationAiportName;
  }

  void setDestinationAiportName(const QString& value)
  {
    destinationAiportName = value;
  }

  bool isEmpty() const
  {
    return entries.isEmpty();
  }

  void clear();

  void setDepartureIdent(const QString& value)
  {
    departureIdent = value;
  }

  void setDestinationIdent(const QString& value)
  {
    destinationIdent = value;
  }

  void setDepartureParkingName(const QString& value)
  {
    departureParkingName = value;
  }

  void setDepartureAiportName(const QString& value)
  {
    departureAiportName = value;
  }

  /*
   * @return destination position/coordinates.
   */
  const atools::geo::Pos& getDestinationPosition() const
  {
    return destinationPos;
  }

  void setDestinationPosition(const atools::geo::Pos& value)
  {
    destinationPos = value;
  }

  /*
   * @return departure position/coordinates.
   */
  const atools::geo::Pos& getDeparturePosition() const
  {
    return departurePos;
  }

  void setDeparturePosition(const atools::geo::Pos& value);
  void setDeparturePosition(const atools::geo::Pos& value, float altitude);

  /*
   * @return title of the flight plan like "EDMA to LESU"
   */
  const QString& getTitle() const
  {
    return title;
  }

  void setTitle(const QString& value)
  {
    title = value;
  }

  /*
   * Reverse start and destination and the order of all entries
   */
  void reverse();

  const QHash<QString, QString>& getProperties() const
  {
    return properties;
  }

  QHash<QString, QString>& getProperties()
  {
    return properties;
  }

  void setCustomData(const QHash<QString, QString>& value)
  {
    properties = value;
  }

  atools::fs::pln::FileFormat getFileFormat() const
  {
    return fileFormat;
  }

  void setFileFormat(const atools::fs::pln::FileFormat& value)
  {
    fileFormat = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::Flightplan& record);

  friend class atools::fs::pln::FlightplanIO;

  /* Values for FSX */
  const QString APPVERSION_BUILD = QString("61472");
  const QString APPVERSION_MAJOR = QString("10");

  /* Limit altitude to this value */
  const int MAX_ALTITUDE = 80000;

  atools::fs::pln::FileFormat fileFormat = PLN_FSX;
  atools::fs::pln::FlightplanType flightplanType = VFR;
  atools::fs::pln::RouteType routeType = DIRECT;

  QList<atools::fs::pln::FlightplanEntry> entries;

  int cruisingAlt;
  QString title, departureIdent, destinationIdent, description,
          departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  atools::geo::Pos departurePos /* Airport or Parking */, destinationPos;

  QHash<QString, QString> properties;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLAN_H
