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

#ifndef ATOOLS_FLIGHTPLANIO_H
#define ATOOLS_FLIGHTPLANIO_H

#include "fs/pln/flightplanconstants.h"

#include <QApplication>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace atools {
namespace util {
class XmlStream;
}
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
   * by the content of the first few lines and supports LNMPLNm FSX/P3D XML, FS9 ini-style, FLP and FMS files.
   *
   * @param file filepath of a valid flight plan file
   * Returns detected file format.
   */
  FileFormat load(atools::fs::pln::Flightplan& plan, const QString& file);

  /* Detect format by reading the first few lines */
  static atools::fs::pln::FileFormat detectFormat(const QString& file);

  /* LNM own XML format
   * Save a flightplan. An exception is thrown if the flight plan contents are not valid.
   * Although the flight simulator cannot deal with flight plans that have no valid start
   * or destination (start or destaintion are merely a waypoint) it is allowed to that save one.
   */
  void saveLnm(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Same as above but returns the LNMPLN in a string */
  QString saveLnmStr(const Flightplan& plan);

  /* Same as above but returns the LNMPLN in a Gzip compressed byte array */
  QByteArray saveLnmGz(const Flightplan& plan);

  /* Load LNMPLN from file */
  void loadLnm(atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Load LNMPLN from string */
  void loadLnmStr(atools::fs::pln::Flightplan& plan, const QString& string);

  /* Load LNMPLN from Gzip compressed byte array */
  void loadLnmGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes);

  /* FSX/P3D XML format */
  void savePln(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* FSX/P3D XML format with annotations as used by previous LNM versions (<= 2.4.5).*/
  void savePlnAnnotated(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* FlightGear route manager XML format */
  void saveFlightGear(const Flightplan& plan, const QString& filename);

  /* PMDG RTE format */
  void saveRte(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Aerosoft Airbus and X-Plane FLP format */
  void saveFlp(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* X-Plane FMS format.
   * @param version11Format Version 11 otherwise 3 */
  void saveFms3(const atools::fs::pln::Flightplan& plan, const QString& file);
  void saveFms11(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* GPX format including track and time stamps if not empty. Number has to match flight plan entry number. */
  void saveGpx(const atools::fs::pln::Flightplan& plan, const QString& filename, const atools::geo::LineString& track,
               const QVector<quint32>& timestamps, int cruiseAltFt);

  /* Same as above but returns the file in a string */
  QString saveGpxStr(const atools::fs::pln::Flightplan& plan, const atools::geo::LineString& track,
                     const QVector<quint32>& timestamps, int cruiseAltFt);

  /* Same as above but returns the file in a Gzip compressed byte array */
  QByteArray saveGpxGz(const atools::fs::pln::Flightplan& plan, const atools::geo::LineString& track,
                       const QVector<quint32>& timestamps, int cruiseAltFt);

  /* Majestic Dash 400 binary format */
  void saveFpr(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* iFly text format .FLTPLAN */
  void saveFltplan(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* PLN for Blackbox Simulations Airbus. Same as FS9 PLN format. */
  void saveBbsPln(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Reality XP GNS XML format. */
  void saveGarminGns(const atools::fs::pln::Flightplan& flightplan, const QString& filename,
                     atools::fs::pln::SaveOptions options);

  /* Feelthere/Wilco Embraer */
  void saveFeelthereFpl(const atools::fs::pln::Flightplan& plan, const QString& filename, int groundSpeed);

  /* Flight plan format for B767 Level-D */
  void saveLeveldRte(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Electronic flight bag */
  void saveEfbr(const atools::fs::pln::Flightplan& plan, const QString& filename, const QString& route,
                const QString& cycle, const QString& departureRw, const QString& destinationRw);

  /* Quality Wings RTE format */
  void saveQwRte(const Flightplan& plan, const QString& filename);

  /* Leonardo Maddog MDX / MDR */
  void saveMdr(const Flightplan& plan, const QString& filename);

  /* TFDi Design 717 XML */
  void saveTfdi(const Flightplan& plan, const QString& filename, const QBitArray& jetAirways);

  /* Version number to save into LNMPLN files */
  static const int LNMPLN_VERSION_MAJOR = 0;
  static const int LNMPLN_VERSION_MINOR = 9;

private:
  void savePlnInternal(const Flightplan& plan, const QString& filename, bool annotated);
  void saveFmsInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool version11Format);
  void saveLnmInternal(QXmlStreamWriter& writer, const Flightplan& plan);
  void saveGpxInternal(const atools::fs::pln::Flightplan& plan, QXmlStreamWriter& writer, const geo::LineString& track,
                       const QVector<quint32>& timestamps, int cruiseAltFt);
  void loadLnmInternal(Flightplan& plan, atools::util::XmlStream& xmlStream);

  /* Load specific formats after content detection */
  void loadFsx(atools::fs::pln::Flightplan& plan, const QString& filename);
  void loadFs9(atools::fs::pln::Flightplan& plan, const QString& filename);
  void loadFlp(atools::fs::pln::Flightplan& plan, const QString& filename);
  void loadFms(atools::fs::pln::Flightplan& plan, const QString& filename);
  void loadFsc(atools::fs::pln::Flightplan& plan, const QString& filename);
  void loadFlightGear(atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Write string into memory location, truncate if needed and fill up to length with null */
  void writeBinaryString(char *mem, QString str, int length);

  QString identOrDegMinFormat(const atools::fs::pln::FlightplanEntry& entry);

  static QString flightplanTypeToString(atools::fs::pln::FlightplanType type);
  static atools::fs::pln::FlightplanType stringFlightplanType(const QString& str);
  static QString routeTypeToString(atools::fs::pln::RouteType type);
  static atools::fs::pln::RouteType stringToRouteType(const QString& str);

  RouteType stringToRouteTypeFs9(const QString& str);
  int routeTypeToStringFs9(atools::fs::pln::RouteType type);

  QString gnsType(const atools::fs::pln::FlightplanEntry& entry);

  void readWaypoint(Flightplan& plan, util::XmlStream& xmlStream);
  void posToRte(QTextStream& stream, const atools::geo::Pos& pos, bool alt);

  /* Support for FlightGear propery lists */
  void writePropertyStr(QXmlStreamWriter& writer, const QString& name, const QString& value);
  void writePropertyBool(QXmlStreamWriter& writer, const QString& name, bool value = true);
  void writePropertyInt(QXmlStreamWriter& writer, const QString& name, int value);
  void writePropertyFloat(QXmlStreamWriter& writer, const QString& name, float value);

  /* Writes element with name if value is not empty */
  void writeElementIf(QXmlStreamWriter& writer, const QString& name, const QString& value);

  /* Writes element "Pos" if pos is valid */
  void writeElementPosIf(QXmlStreamWriter& writer, const atools::geo::Pos& pos);

  /* Inserts property if value is not empty */
  void insertPropertyIf(Flightplan& plan, const QString& key, const QString& value);

  /* Read "Pos" element and attributes from stream in LNM XML format */
  atools::geo::Pos readPosLnm(QXmlStreamReader& reader);

  /* Read waypoint elements and attributes from stream */
  void readWaypointsLnm(atools::util::XmlStream& xmlStream, QList<FlightplanEntry>& entries,
                        const QString& elementName);

  /* Set altitude in all positions */
  void assignAltitudeToAllEntries(Flightplan& plan);

  /* Number of entries including start and destination but excluding procedure points */
  int numEntriesSave(const Flightplan& plan);

  /* Copy departure and destination from first and last entry */
  void adjustDepartureAndDestination(atools::fs::pln::Flightplan& plan);

  QString coordStringFs9(const atools::geo::Pos& pos);
  void writeWaypointLnm(QXmlStreamWriter& writer, const FlightplanEntry& entry, const QString& elementName);

  QString errorMsg;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANIO_H
