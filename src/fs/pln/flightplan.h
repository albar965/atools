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

namespace gpx {
class GpxIO;
}
namespace pln {

class FlightplanIO;

/*
 * A class to load, modify and save FSX (and all other compatible simulators) flight plans.
 */
class Flightplan :
  private QList<atools::fs::pln::FlightplanEntry>
{
  Q_DECLARE_TR_FUNCTIONS(Flightplan)

public:
  /* Prefill flight plan from departure and destination airport data. */
  void setDepartureDestination(const QString& departIdent, const atools::geo::Pos& departPos,
                               const QString& destIdent, const atools::geo::Pos& destPos);

  /* Total distance for all waypoints */
  float getDistanceNm() const;

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
  float getCruiseAltitudeFt() const
  {
    return cruiseAltitudeFt;
  }

  void setCruiseAltitudeFt(float value)
  {
    cruiseAltitudeFt = value;
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

  /* Pull required methods into public space */
  /* Keep clear() hidden */
  using QList::append;
  using QList::at;
  using QList::begin;
  using QList::constBegin;
  using QList::constEnd;
  using QList::constFirst;
  using QList::constLast;
  using QList::end;
  using QList::erase;
  using QList::first;
  using QList::insert;
  using QList::isEmpty;
  using QList::last;
  using QList::move;
  using QList::operator[];
  using QList::prepend;
  using QList::rbegin;
  using QList::removeAt;
  using QList::rend;
  using QList::replace;
  using QList::size;

  void clearAll();

  void clearEntries()
  {
    QList::clear();
  }

  void clearProperties()
  {
    properties.clear();
  }

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
   * @return departure parking name like "PARKING 2" or "27" for runway. Empty for type AIRPORT.
   * Zero prefixed number like "01" for helipad.
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

  void clearDepartureParking();

  const QHash<QString, QString>& getProperties() const
  {
    return properties;
  }

  QHash<QString, QString>& getProperties()
  {
    return properties;
  }

  const QHash<QString, QString>& getPropertiesConst() const
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

  /* EINN to ENKR at 10000 ft, VFR */
  QString getDescription() const;

  /* "FROM to TO ... created by ..." */
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
  QString getFilenamePattern(const QString& pattern, const QString& suffix, bool metric) const;

  static QString getFilenamePattern(QString pattern, const QString& type, const QString& departureName, const QString& departureIdent,
                                    const QString& destName, const QString& destIdent, const QString& suffix, int altitudeLocal);

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

  /* Pointer to alternates are valid until flight plan is modified */
  const QVector<const FlightplanEntry *> getAlternates() const;

  /* Clear out all entries with no save, alternate or procedure = true */
  void removeIsNoSaveEntries()
  {
    erase(std::remove_if(begin(), end(), [](const FlightplanEntry& type) -> bool {
            return type.isNoSave();
          }), end());
  }

  void removeAlternateEntries()
  {
    erase(std::remove_if(begin(), end(), [](const FlightplanEntry& type) -> bool {
            return type.isAlternate();
          }), end());
  }

  void removeProcedureEntries()
  {
    erase(std::remove_if(begin(), end(), [](const FlightplanEntry& type) -> bool {
            return type.isProcedure();
          }), end());
  }

  static QString flightplanTypeToString(atools::fs::pln::FlightplanType type);
  static atools::fs::pln::FlightplanType stringFlightplanType(const QString& str);
  static QString routeTypeToString(atools::fs::pln::RouteType type);
  static atools::fs::pln::RouteType stringToRouteType(const QString& str);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::Flightplan& record);

  /* Required to fill this structure */
  friend class atools::fs::pln::FlightplanIO;
  friend class atools::fs::gpx::GpxIO;

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

  float cruiseAltitudeFt;
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
