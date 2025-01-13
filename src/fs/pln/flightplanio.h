/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

  /*
   * Load a flightplan. An exception is thrown if the file is not valid. The type will be detected automatically
   * by the content of the first few lines and supports LNMPLNm FSX/P3D XML, FS9 ini-style, FLP and FMS files.
   *
   * @param file filepath of a valid flight plan file
   * Returns detected file format.
   */
  FileFormat load(atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* Detect format by reading the first few lines */
  static atools::fs::pln::FileFormat detectFormat(const QString& file);

  /* true for any supported flight plan file */
  static bool isFlightplanFile(const QString& file)
  {
    return detectFormat(file) != atools::fs::pln::NONE;
  }

  /* LNM own XML format
   * Save a flightplan. An exception is thrown if the flight plan contents are not valid.
   * Although the flight simulator cannot deal with flight plans that have no valid start
   * or destination (start or destaintion are merely a waypoint) it is allowed to that save one.
   */
  void saveLnm(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Same as above but returns the LNMPLN in a string */
  QString saveLnmStr(const Flightplan& plan) const;

  /* Same as above but returns the LNMPLN in a Gzip compressed byte array */
  QByteArray saveLnmGz(const Flightplan& plan) const;

  /* Load LNMPLN from file */
  void loadLnm(atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Load LNMPLN from string */
  void loadLnmStr(atools::fs::pln::Flightplan& plan, const QString& string) const;

  /* Load LNMPLN from Gzip compressed byte array */
  void loadLnmGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes) const;

  /* FSX/P3D XML format */
  void savePln(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* Microsoft Flight Simulator 2020 with star as degree separator */
  void savePlnMsfs(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* Microsoft Flight Simulator 2024 */
  void savePlnMsfs24(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* Microsoft Flight Simulator 2020 with ° as degree separator for add-ons */
  void savePlnMsfsCompat(const Flightplan& plan, const QString& file) const;

  /* MSFS PLN for Integrated Simavionics / ISG */
  void savePlnIsg(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* PMS50 GTN750 */
  void savePlnPms50(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* FlightGear route manager XML format */
  void saveFlightGear(const Flightplan& plan, const QString& filename) const;

  /* PMDG RTE format */
  void saveRte(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Aerosoft Airbus and X-Plane FLP format */
  void saveFlp(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Aerosoft CRJ FLP format */
  void saveCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Aerosoft CRJ for MSFS FLP format */
  void saveMsfsCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* X-Plane FMS format.
   * @param version11Format Version 11 otherwise 3 */
  void saveFms3(const atools::fs::pln::Flightplan& plan, const QString& file) const;
  void saveFms11(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* X-Plane CIVA Navigation System. Saves multiple files. */
  void saveCivaFms(Flightplan plan, const QString& file) const;

  /* MSFS IniBuilds A310 */
  void saveIniBuildsMsfs(const atools::fs::pln::Flightplan& plan, const QString& file) const;

  /* Garmin FPL (XML) format for Reality XP GNS XML. */
  void loadGarminFpl(atools::fs::pln::Flightplan& plan, const QString& filename) const;
  void saveGarminFpl(Flightplan flightplan, const QString& filename, bool saveAsUserWaypoints) const;

  /* Garmin GFP - One line text format prefixed with "FPN/RI:..." */
  void loadGarminGfp(atools::fs::pln::Flightplan& plan, const QString& filename) const;

  void loadGarminFplStr(atools::fs::pln::Flightplan& plan, const QString& string) const;
  void loadGarminFplGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes) const;

  /* Export only formats below ================================================================ */

  /* Majestic Dash 400 binary format */
  void saveFpr(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* iFly text format .FLTPLAN */
  void saveFltplan(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* PLN for Blackbox Simulations Airbus. Same as FS9 PLN format. */
  void saveBbsPln(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Feelthere/Wilco Embraer */
  void saveFeelthereFpl(const atools::fs::pln::Flightplan& plan, const QString& filename, int groundSpeed) const;

  /* Flight plan format for B767 Level-D */
  void saveLeveldRte(const atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Electronic flight bag */
  void saveEfbr(const atools::fs::pln::Flightplan& plan, const QString& filename, const QString& route,
                const QString& cycle, const QString& departureRw, const QString& destinationRw) const;

  /* Quality Wings RTE format */
  void saveQwRte(const Flightplan& plan, const QString& filename) const;

  /* Leonardo Maddog MDX / MDR */
  void saveMdr(const Flightplan& plan, const QString& filename) const;

  /* TFDi Design 717 XML */
  void saveTfdi(const Flightplan& plan, const QString& filename, const QBitArray& jetAirways) const;

  /*  iFly Jets Advanced Series */
  void saveIfly(const Flightplan& plan, const QString& filename) const;

  /* Version number to save into LNMPLN files */
  static const int LNMPLN_VERSION_MAJOR = 1;
  static const int LNMPLN_VERSION_MINOR = 2;

private:
  void savePlnInternal(const Flightplan& plan, const QString& filename, bool msfs, bool msfs24, bool simavionics, bool pms50, bool starDeg,
                       int userWpLength) const;
  void saveFmsInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool version11Format, bool iniBuildsFormat) const;
  void saveLnmInternal(QXmlStreamWriter& writer, const Flightplan& plan) const;
  void saveFlpInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool crj, bool msfs) const;
  void loadLnmInternal(Flightplan& plan, atools::util::XmlStream& xmlStream) const;
  void loadGarminFplInternal(Flightplan& plan, atools::util::XmlStream& xmlStream) const;
  atools::fs::pln::entry::WaypointType garminToWaypointType(const QString& typeStr) const;

  /* Load specific formats after content detection */
  void loadPln(atools::fs::pln::Flightplan& plan, const QString& filename, FileFormat format) const; /* FSX, P3D, MSFS 2020 and MSFS 2024 */
  void loadFs9(atools::fs::pln::Flightplan& plan, const QString& filename) const;
  void loadFlp(atools::fs::pln::Flightplan& plan, const QString& filename) const;
  void loadFms(atools::fs::pln::Flightplan& plan, const QString& filename) const; /* X-Plane 11 and 12 format 3 and 11 */
  void loadFsc(atools::fs::pln::Flightplan& plan, const QString& filename) const;
  void loadFlightGear(atools::fs::pln::Flightplan& plan, const QString& filename) const;

  /* Write string into memory location, truncate if needed and fill up to length with null */
  void writeBinaryString(char *mem, QString str, int length) const;

  QString identOrDegMinFormat(const atools::fs::pln::FlightplanEntry& entry) const;

  RouteType stringToRouteTypeFs9(const QString& str) const;
  int routeTypeToStringFs9(atools::fs::pln::RouteType type) const;

  QString gnsType(const atools::fs::pln::FlightplanEntry& entry) const;

  void readWaypointPln(Flightplan& plan, atools::util::XmlStream& xmlStream) const;
  void readAppVersionPln(int& appVersionMajor, int& appVersionBuild, atools::util::XmlStream& xmlStream) const;
  void posToRte(QTextStream& stream, const atools::geo::Pos& pos, bool alt) const;

  /* Support for FlightGear propery lists */
  void writePropertyStr(QXmlStreamWriter& writer, const QString& name, const QString& value) const;
  void writePropertyBool(QXmlStreamWriter& writer, const QString& name, bool value = true) const;
  void writePropertyInt(QXmlStreamWriter& writer, const QString& name, int value) const;
  void writePropertyFloat(QXmlStreamWriter& writer, const QString& name, float value) const;

  /* Writes element with name if value is not empty */
  void writeTextElementIf(QXmlStreamWriter& writer, const QString& name, const QString& value) const;

  /* Writes element "Pos" if pos is valid */
  void writeElementPosIf(QXmlStreamWriter& writer, const atools::geo::Pos& pos) const;

  /* Inserts property if value is not empty */
  void insertPropertyIf(Flightplan& plan, const QString& key, const QString& value) const;

  /* Read "Pos" element and attributes from stream in LNM XML format */
  atools::geo::Pos readPosLnm(atools::util::XmlStream& xmlStream) const;

  /* Read waypoint elements and attributes from stream */
  void readWaypointsLnm(atools::util::XmlStream& xmlStream, QList<FlightplanEntry>& entries,
                        const QString& elementName) const;

  /* Number of entries including start and destination but excluding procedure points */
  int numEntriesSave(const Flightplan& plan) const;

  QString coordStringFs9(const atools::geo::Pos& pos) const;
  void writeWaypointLnm(QXmlStreamWriter& writer, const FlightplanEntry& entry, const QString& elementName) const;

  QString approachToMsfs(const QString& type) const;
  QString msfsToApproachType(const QString& type) const;

  void saveFlpKeyValue(QTextStream& stream, const atools::fs::pln::Flightplan& plan, const QString& prefix,
                       const QString& key, const QString& property) const;

  /* Add zero prefix for X-Plane runway numbers */
  QString xplaneRunway(QString runway) const;

  QString errorMsg;

};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANIO_H
