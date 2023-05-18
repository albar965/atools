/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include <QCoreApplication>

class QXmlStreamReader;

namespace atools {
namespace geo {
class LineString;
}
namespace fs {
namespace pln {

class FlightplanIO;

typedef QList<atools::fs::pln::FlightplanEntry> FlightplanEntryListType;

/*
 * A class to load, modify and save FSX (and all other compatible simulators) flight plans.
 */
class Flightplan
{
  Q_DECLARE_TR_FUNCTIONS(Flightplan)

public:
  /*
   * @return Get all flight plan entries/waypoints. These include start and destination.
   */
  atools::fs::pln::FlightplanEntryListType& getEntries()
  {
    return entries;
  }

  /* Clear out all entries with no save = true */
  void removeNoSaveEntries();

  /*
   * @return Get all flight plan entries/waypoints. These include start and destination.
   */
  const atools::fs::pln::FlightplanEntryListType& getEntries() const
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

  /* IFR or VFR */
  QString getFlightplanTypeStr() const;

  void setFlightplanType(atools::fs::pln::FlightplanType value)
  {
    flightplanType = value;
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

  const QString& getDepartureName() const
  {
    return departureName;
  }

  const QString& getDestinationName() const
  {
    return destinationName;
  }

  void setDestinationName(const QString& value)
  {
    destinationName = value;
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

  void setDepartureName(const QString& value)
  {
    departureName = value;
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
   * @return airport departure position/coordinates.
   */
  const atools::geo::Pos& getDeparturePosition() const
  {
    return departurePos;
  }

  void setDeparturePosition(const atools::geo::Pos& value);
  void setDeparturePosition(const atools::geo::Pos& value, float altitude);

  /*
   * @return departure parking name like "PARKING 2"
   */
  const QString& getDepartureParkingName() const
  {
    return departureParkingName;
  }

  /*
   * @return parking, runway or airport departure position/coordinates.
   */
  const atools::geo::Pos& getDepartureParkingPosition() const
  {
    return departureParkingPos.isValid() ? departureParkingPos : departurePos;
  }

  /* Set altitude and heading separately if not INVALID_ALTITUDE and not INVALID_HEADING */
  void setDepartureParkingPosition(const atools::geo::Pos& value, float altitudeFt, float headingTrue);

  /* Type of parking position. This is set by the program after loading a plan and is needed for export. */
  atools::fs::pln::FlightplanParkingType getDepartureParkingType() const
  {
    return departureParkingType;
  }

  /* True heading at parking spot if available or INVALID_HEADING */
  float getDepartureParkingHeading() const
  {
    return departureParkingHeading;
  }

  QString getDepartureParkingTypeStr() const;

  void setDepartureParkingType(atools::fs::pln::FlightplanParkingType value)
  {
    departureParkingType = value;
  }

  void setDepartureParkingType(QString type);

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

  const QString& getComment() const
  {
    return comment;
  }

  void setComment(const QString& value)
  {
    comment = value;
  }

  QString getTitle() const
  {
    return departureIdent + " to " + destinationIdent;
  }

  QString getDescr() const;

  /* true if this was detected and loaded from LNMPLN */
  bool isLnmFormat() const
  {
    return lnmFormat;
  }

  void setLnmFormat(bool value)
  {
    lnmFormat = value;
  }

  /*
   * Build a filename according to pattern and add suffix.
   * Filename will be cleaned of invalid characters if clean is set.
   *
   * See variable definitions in namespace atools::fs::pln::pattern.
   */
  QString getFilenamePattern(const QString& pattern, const QString& suffix, bool clean = true) const;

  static QString getFilenamePattern(QString pattern, const QString& type, const QString& departureName, const QString& departureIdent,
                                    const QString& destName, const QString& destIdent, const QString& suffix, int altitude,
                                    bool clean = true);

  /* Return an example flight plan name for the given pattern plus optional error message in case of invalid characters.
   * Message can be formatted for HTML output. */
  static QString getFilenamePatternExample(const QString& pattern, const QString& suffix, bool html,
                                           QString *errorMessage = nullptr);

  /* Direct, LowAlt, etc. only used for MS simulators */
  atools::fs::pln::RouteType getRouteType() const
  {
    return routeType;
  }

  void setRouteType(atools::fs::pln::RouteType value)
  {
    routeType = value;
  }

  /* Copy departure and destination from first and last entry */
  void adjustDepartureAndDestination(bool force = false);

  /* Set altitude in all positions */
  void assignAltitudeToAllEntries();

  /* Removes all redundant airway waypoints and returns a copy of this.
   * Only to be used for export since the plan is not consistent afterwards.
   * Converts wp1 -> AW1 -> wp2 -> AW1 -> wp3 -> AW1 -> wp4 to wp1 -> AW1 -> wp4 */
  atools::fs::pln::Flightplan compressedAirways() const;

  /* Creates a simple waypoint/airway string from the flight plan.
   * Example: EDDF NOKDI [Y163] NATOR [N850] TITIX KEMMI DORAV IXITO [L50] ELB [M729] MEDAL [Q160] OST LIRF
   * To be used for testing and logging */
  QString toShortString() const;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::Flightplan& record);

  friend class atools::fs::pln::FlightplanIO;

  const QString& departNameOrIdent() const
  {
    return departureName.isEmpty() ? departureIdent : departureName;
  }

  const QString& destNameOrIdent() const
  {
    return destinationName.isEmpty() ? destinationIdent : destinationName;
  }

  /* get the first airport which has the no-save flag not set, i.e. the last airport before the alternates */
  const FlightplanEntry& destinationAirport() const;

  /* Limit altitude to this value */
  static const int MAX_ALTITUDE = 80000;

  atools::fs::pln::FlightplanType flightplanType = VFR;
  atools::fs::pln::RouteType routeType = DIRECT;

  atools::fs::pln::FlightplanEntryListType entries;

  int cruisingAlt;
  QString departureIdent, destinationIdent,
          departureParkingName, departureName, destinationName, comment;
  atools::geo::Pos departurePos /* Airport */,
                   departureParkingPos /* Parking, runway or airport */,
                   destinationPos; /* Always airport */

  float departureParkingHeading = atools::fs::pln::INVALID_HEADING;

  atools::fs::pln::FlightplanParkingType departureParkingType = NO_POS;

  bool lnmFormat = true;

  QHash<QString, QString> properties;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLAN_H
