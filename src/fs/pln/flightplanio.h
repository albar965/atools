/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FLIGHTPLANIO_H
#define ATOOLS_FLIGHTPLANIO_H

#include "fs/pln/flightplanconstants.h"

#include <QApplication>

class QXmlStreamReader;

namespace atools {
namespace geo {
class LineString;
class Pos;
}
namespace fs {
namespace pln {

class Flightplan;
class FlightplanEntry;

/*
 * Collects all save and load methods of flight plan.
 * Stateless except filename for error reporting.
 */
class FlightplanIO
{
  Q_DECLARE_TR_FUNCTIONS(FlightplanIO)

public:
  FlightplanIO();
  ~FlightplanIO();

  /*
   * Load a flightplan. An exception is thrown if the file is not valid. The type will be detected automatically
   * by the content of the first few lines and supports FSX/P3D XML, FS9 ini-style, FLP and FMS files.
   *
   * @param file filepath of a valid flight plan file
   */
  void load(atools::fs::pln::Flightplan& plan, const QString& file);

  /*
   * Save a flightplan. An exception is thrown if the flight plan contents are not valid.
   * Although the flight simulator cannot deal with flight plans that have no valid start
   * or destination (start or destaintion are merely a waypoint) it is allowed to that save one.
   *
   * The plan is saved in the same format as loaded, FSX/P3D, FLP or FMS.
   *
   * @param file filepath of the file to be saved
   * @param clean if true save all properties in a XML comment
   */
  void save(const atools::fs::pln::Flightplan& flightplan, const QString& file, const QString& airacCycle,
            atools::fs::pln::SaveOptions options);

  /* FSX/P3D XML format */
  void saveFsx(const atools::fs::pln::Flightplan& plan, const QString& file,
               atools::fs::pln::SaveOptions options);

  /* PMDG RTE format */
  void saveRte(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* Aerosoft Airbus and X-Plane FLP format */
  void saveFlp(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* X-Plane FMS format.
   * @param version11Format Version 11 otherwise 3 */
  void saveFms(const atools::fs::pln::Flightplan& plan, const QString& file,
               const QString& airacCycle,
               bool version11Format);

  /* GPX format including track and time stamps if not empty. Number has to match flight plan entry number. */
  void saveGpx(const atools::fs::pln::Flightplan& plan, const QString& file, const atools::geo::LineString& track,
               const QVector<quint32>& timestamps, int cruiseAltFt);

  /* Majestic Dash 400 binary format */
  void saveFpr(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* iFly text format .FLTPLAN */
  void saveFltplan(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* PLN for Blackbox Simulations Airbus. Same as FS9 PLN format. */
  void saveBbsPln(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* Reality XP GNS XML format. */
  void saveGarminGns(const atools::fs::pln::Flightplan& flightplan, const QString& file,
                     atools::fs::pln::SaveOptions options);

  static const QStringList& getAcceptedFlightPlanExtensions();

private:
  /* Get the first four lines of a file converted to lowercase to check type.
   *  Returns a list with always four strings. */
  QStringList probeFile4(const QString& file);

  /* Load specific formats after content detection */
  void loadFsx(atools::fs::pln::Flightplan& plan, const QString& file);
  void loadFs9(atools::fs::pln::Flightplan& plan, const QString& file);
  void loadFlp(atools::fs::pln::Flightplan& plan, const QString& file);
  void loadFms(atools::fs::pln::Flightplan& plan, const QString& file);
  void loadFsc(atools::fs::pln::Flightplan& plan, const QString& file);

  /* Write string into memory location, truncate if needed and fill up to length with null */
  void writeBinaryString(char *mem, QString str, int length);

  static QString flightplanTypeToString(atools::fs::pln::FlightplanType type);
  static atools::fs::pln::FlightplanType stringFlightplanType(const QString& str);
  static QString routeTypeToString(atools::fs::pln::RouteType type);
  static atools::fs::pln::RouteType stringToRouteType(const QString& str);

  RouteType stringToRouteTypeFs9(const QString& str);
  int routeTypeToStringFs9(atools::fs::pln::RouteType type);

  QString gnsType(const atools::fs::pln::FlightplanEntry& entry);

  void readUntilElement(QXmlStreamReader& reader, const QString& name);
  void readAppVersion(Flightplan& plan, QXmlStreamReader& reader);
  void readWaypoint(Flightplan& plan, QXmlStreamReader& reader);
  void posToRte(QTextStream& stream, const atools::geo::Pos& pos, bool alt);

  /* Set altitude in all positions */
  void assignAltitudeToAllEntries(Flightplan& plan);

  /* Number of entries including start and destination but excluding procedure points */
  int numEntriesSave(const Flightplan& plan);

  /* Copy departure and destination from first and last entry */
  void adjustDepartureAndDestination(atools::fs::pln::Flightplan& plan);

  QString coordStringFs9(const atools::geo::Pos& pos);

  QString filename;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANIO_H
