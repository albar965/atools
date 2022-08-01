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

#include <QCoreApplication>

class QXmlStreamReader;
class QXmlStreamWriter;
class QTextStream;

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

  /* Microsoft Flight Simulator 2020 */
  void savePlnMsfs(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* MSFS PLN for Integrated Simavionics / ISG */
  void savePlnIsg(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* FSX/P3D XML format with annotations as used by previous LNM versions (<= 2.4.5).*/
  void savePlnAnnotated(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* FlightGear route manager XML format */
  void saveFlightGear(const Flightplan& plan, const QString& filename);

  /* PMDG RTE format */
  void saveRte(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Aerosoft Airbus and X-Plane FLP format */
  void saveFlp(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Aerosoft CRJ FLP format */
  void saveCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* Aerosoft CRJ for MSFS FLP format */
  void saveMsfsCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* X-Plane FMS format.
   * @param version11Format Version 11 otherwise 3 */
  void saveFms3(const atools::fs::pln::Flightplan& plan, const QString& file);
  void saveFms11(const atools::fs::pln::Flightplan& plan, const QString& file);

  /* GPX format including track and time stamps if not empty. Number has to match flight plan entry number. */
  void saveGpx(const atools::fs::pln::Flightplan& plan, const QString& filename, const QVector<geo::LineString>& tracks,
               const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt);

  /* Same as above but returns the file in a string */
  QString saveGpxStr(const atools::fs::pln::Flightplan& plan, const QVector<geo::LineString>& tracks,
                     const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt);

  /* Same as above but returns the file in a Gzip compressed byte array */
  QByteArray saveGpxGz(const atools::fs::pln::Flightplan& plan, const QVector<geo::LineString>& tracks,
                       const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt);

  /* Loads GPX route coordinates and track points into LineStrings.
   * Reading is limited to files exported by this class.
   * track, route and routenames can be null and will be ignored then */
  void loadGpxStr(atools::geo::LineString *route, QStringList *routenames, QVector<geo::LineString> *tracks,
                  QVector<QVector<qint64> > *timestampsMs, const QString& string);
  void loadGpxGz(atools::geo::LineString *route, QStringList *routenames, QVector<geo::LineString> *tracks,
                 QVector<QVector<qint64> > *timestampsMs, const QByteArray& bytes);
  void loadGpx(atools::geo::LineString *route, QStringList *routenames, QVector<geo::LineString> *tracks,
               QVector<QVector<qint64> > *timestampsMs, const QString& filename);

  /* Garmin FPL (XML) format for Reality XP GNS XML. */
  void loadGarminFpl(atools::fs::pln::Flightplan& plan, const QString& filename);
  void saveGarminFpl(const atools::fs::pln::Flightplan& flightplan, const QString& filename, bool saveAsUserWaypoints);

  /* Garmin GFP - One line text format prefixed with "FPN/RI:..." */
  void loadGarminGfp(atools::fs::pln::Flightplan& plan, const QString& filename);

  void loadGarminFplStr(atools::fs::pln::Flightplan& plan, const QString& string);
  void loadGarminFplGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes);

  /* Export only formats below ================================================================ */

  /* Majestic Dash 400 binary format */
  void saveFpr(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* iFly text format .FLTPLAN */
  void saveFltplan(const atools::fs::pln::Flightplan& plan, const QString& filename);

  /* PLN for Blackbox Simulations Airbus. Same as FS9 PLN format. */
  void saveBbsPln(const atools::fs::pln::Flightplan& plan, const QString& filename);

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

  /*  iFly Jets Advanced Series */
  void saveIfly(const Flightplan& plan, const QString& filename);

  /* Version number to save into LNMPLN files */
  static const int LNMPLN_VERSION_MAJOR = 1;
  static const int LNMPLN_VERSION_MINOR = 1;

private:
  void savePlnInternal(const Flightplan& plan, const QString& filename, bool annotated, bool msfs, int userWpLength);
  void saveFmsInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool version11Format);
  void saveLnmInternal(QXmlStreamWriter& writer, const Flightplan& plan);
  void saveGpxInternal(const atools::fs::pln::Flightplan& plan, QXmlStreamWriter& writer,
                       const QVector<geo::LineString>& tracks, const QVector<QVector<qint64> >& timestampsMs,
                       int cruiseAltFt);
  void saveFlpInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool crj, bool msfs);
  void loadLnmInternal(Flightplan& plan, atools::util::XmlStream& xmlStream);
  void loadGpxInternal(atools::geo::LineString *route, QStringList *routenames, QVector<geo::LineString> *tracks,
                       QVector<QVector<qint64> > *timestampsMs, util::XmlStream& xmlStream);
  void loadGarminFplInternal(Flightplan& plan, util::XmlStream& xmlStream);
  atools::fs::pln::entry::WaypointType garminToWaypointType(const QString& typeStr) const;

  /* Load specific formats after content detection */
  void loadPln(atools::fs::pln::Flightplan& plan, const QString& filename);
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

  void readWaypointPln(Flightplan& plan, util::XmlStream& xmlStream);
  void readAppVersionPln(int& appVersionMajor, int& appVersionBuild, atools::util::XmlStream& xmlStream);
  void posToRte(QTextStream& stream, const atools::geo::Pos& pos, bool alt);

  /* Support for FlightGear propery lists */
  void writePropertyStr(QXmlStreamWriter& writer, const QString& name, const QString& value);
  void writePropertyBool(QXmlStreamWriter& writer, const QString& name, bool value = true);
  void writePropertyInt(QXmlStreamWriter& writer, const QString& name, int value);
  void writePropertyFloat(QXmlStreamWriter& writer, const QString& name, float value);

  /* Writes element with name if value is not empty */
  void writeTextElementIf(QXmlStreamWriter& writer, const QString& name, const QString& value);

  /* Writes element "Pos" if pos is valid */
  void writeElementPosIf(QXmlStreamWriter& writer, const atools::geo::Pos& pos);

  /* Inserts property if value is not empty */
  void insertPropertyIf(Flightplan& plan, const QString& key, const QString& value);

  /* Read "Pos" element and attributes from stream in LNM XML format */
  atools::geo::Pos readPosLnm(util::XmlStream& xmlStream);
  void readPosGpx(geo::Pos& pos, QString& name, QDateTime& timestamp, util::XmlStream& xmlStream);

  /* Read waypoint elements and attributes from stream */
  void readWaypointsLnm(atools::util::XmlStream& xmlStream, QList<FlightplanEntry>& entries,
                        const QString& elementName);

  /* Number of entries including start and destination but excluding procedure points */
  int numEntriesSave(const Flightplan& plan);

  QString coordStringFs9(const atools::geo::Pos& pos);
  void writeWaypointLnm(QXmlStreamWriter& writer, const FlightplanEntry& entry, const QString& elementName);

  QString approachToMsfs(const QString& type);
  QString msfsToApproachType(const QString& type);

  void saveFlpKeyValue(QTextStream& stream, const atools::fs::pln::Flightplan& plan, const QString& prefix,
                       const QString& key, const QString& property);

  QString errorMsg;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANIO_H
