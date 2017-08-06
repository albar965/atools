/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include <QApplication>

class QXmlStreamReader;

namespace atools {
namespace geo {
class LineString;
}
namespace fs {
namespace pln {

enum SourceFile
{
  NONE,
  FSX_P3D, // FSX or P3D XML PLN flight plan
  FS9, // FS9 ini style PLN flight plan
  FMS, // X-Plane FMS file
  FLP // Aerosoft airbus or FlightFactor Boeing
};

enum FlightplanType
{
  IFR,
  VFR
};

enum RouteType
{
  LOW_ALTITUDE,
  HIGH_ALTITUDE,
  VOR, /* Used for radio navaid routing (VOR and NDB) */
  DIRECT, /* Direct connection without waypoints */
  UNKNOWN /* Has to be changed later when resolving the ident to database objects */
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"

/* Common key that are used int flight plan properties that are not supported in PLN.
 * Will be save inside a XML comment in pln files. */
/* Keys that describe procedures*/
const QLatin1Literal SIDAPPR("sidappr");
const QLatin1Literal SIDAPPRRW("sidapprrw");
const QLatin1Literal SIDAPPRDISTANCE("sidapprdistance");
const QLatin1Literal SIDAPPRSIZE("sidapprsize");
const QLatin1Literal SIDTRANS("sidtrans");
const QLatin1Literal SIDTRANSDISTANCE("sidtransdistance");
const QLatin1Literal SIDTRANSSIZE("sidtranssize");
const QLatin1Literal STAR("star");
const QLatin1Literal STARDISTANCE("stardistance");
const QLatin1Literal STARSIZE("starsize");
const QLatin1Literal STARTRANS("startrans");
const QLatin1Literal STARTRANSDISTANCE("startransdistance");
const QLatin1Literal STARTRANSSIZE("startranssize");
const QLatin1Literal TRANSITION("transition");
const QLatin1Literal TRANSITIONTYPE("transitiontype");
const QLatin1Literal TRANSITIONDISTANCE("transitiondistance");
const QLatin1Literal TRANSITIONSIZE("transitionsize");
const QLatin1Literal APPROACH("approach");
const QLatin1Literal APPROACHTYPE("approachtype");
const QLatin1Literal APPROACHRW("approachrw");
const QLatin1Literal APPROACHSUFFIX("approachsuffix");
const QLatin1Literal APPROACHDISTANCE("approachdistance");
const QLatin1Literal APPROACHSIZE("approachsize");

/* Speed as is not supported by PLN format */
const QLatin1Literal SPEED("speed");

/* Free parking spot name as not supported by PLN */
const QLatin1Literal PARKING("parking");

#pragma GCC diagnostic pop

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
   * Load a flightplan. An exception is thrown if the file is not valid. The type will be detected automatically
   * by the content of the first few lines and supports FSX/P3D XML, FS9 ini-style, FLP and FMS files.
   *
   * @param file filepath of a valid flight plan file
   */
  void load(const QString& file);

  /*
   * Save a flightplan. An exception is thrown if the flight plan contents are not valid.
   * Although the flight simulator cannot deal with flight plans that have no valid start
   * or destination (start or destaintion are merely a waypoint) it is allowed to that save one.
   * @param file filepath of the file to be saved
   * @param clean if true save all properties in a XML comment
   */
  void save(const QString& file, bool clean = false);

  /* PMDG RTE format */
  void saveRte(const QString& file);

  /* Aerosoft Airbus FLP format */
  void saveFlp(const QString& file, bool saveProcedures);

  /* X-Plane FMS format */
  void saveFms(const QString& file);

  /* GPX format including track and time stamps if not empty. Number has to match flight plan entry number. */
  void saveGpx(const QString& file, const geo::LineString& track, const QVector<quint32>& timestamps);

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

  void setDeparturePosition(const atools::geo::Pos& value)
  {
    departurePos = value;
  }

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

  atools::fs::pln::SourceFile getSource() const
  {
    return source;
  }

  void setSource(const atools::fs::pln::SourceFile& value)
  {
    source = value;
  }

private:
  void loadFsx(const QString& file);
  void loadFs9(const QString& file);
  void loadFlp(const QString& file);
  void loadFms(const QString& file);

  static QString flightplanTypeToString(atools::fs::pln::FlightplanType type);
  static atools::fs::pln::FlightplanType stringFlightplanType(const QString& str);
  static QString routeTypeToString(atools::fs::pln::RouteType type);
  static atools::fs::pln::RouteType stringToRouteType(const QString& str);
  RouteType stringToRouteTypeFs9(const QString& str);

  void readUntilElement(QXmlStreamReader& reader, const QString& name);
  void readAppVersion(QXmlStreamReader& reader);
  void readWaypoint(QXmlStreamReader& reader);
  void posToRte(QTextStream& stream, const geo::Pos& pos, bool alt);
  QString programInfo();

  /* Get the first four lines of a file to check type */
  QStringList probeFile(const QString& file);

  /* Copy departure and destination from first and last entry */
  void adjustDepartureAndDestination();

  /* Values for FSX */
  const QString APPVERSION_BUILD = QString("61472");
  const QString APPVERSION_MAJOR = QString("10");

  atools::fs::pln::SourceFile source = NONE;
  atools::fs::pln::FlightplanType flightplanType = VFR;
  atools::fs::pln::RouteType routeType = DIRECT;

  QList<atools::fs::pln::FlightplanEntry> entries;

  int cruisingAlt;
  QString filename, title, departureIdent, destinationIdent, description,
          departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  atools::geo::Pos departurePos, destinationPos;

  QHash<QString, QString> properties;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLAN_H
