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

#ifndef ATOOLS_XP_DATAWRITER_H
#define ATOOLS_XP_DATAWRITER_H

#include "fs/xp/xpconstants.h"

#include <QApplication>

class QTextStream;
class QFile;

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
class NavDatabaseOptions;
class NavDatabaseErrors;
class ProgressHandler;

namespace xp {

class XpFixWriter;
class XpNavWriter;
class XpAirwayWriter;
class XpAirportWriter;
class XpCifpWriter;
class XpWriter;
class AirwayPostProcess;
class XpAirportIndex;

/*
 * Provides methods to read X-Plane data from text files into the database.
 * Reads either from $X-Plane/Resources/default data/ or $X-Plane/Custom Data/
 *
 * Base layer
 * $X-Plane/Resources/default data/
 * earth_fix.dat
 * earth_awy.dat
 * earth_nav.dat
 * CIFP/$ICAO.dat (where $ICAO is each airport with instrument procedures)
 *
 * Updated base
 * If these files are present, the X-Plane base layer is ignored.
 * $X-Plane/Custom Data/
 * earth_fix.dat
 * earth_awy.dat
 * earth_nav.dat
 * CIFP/$ICAO.dat (where $ICAO is each airport with instrument procedures)
 *
 * In X-Plane 11, this file is used to replace P* records with the latest from the FAA.
 * $X-Plane/Custom Data/
 * FAACIFP18
 * Note that no enroute waypoints, VHF enroute navaids, or enroute airways are loaded from this file.
 * These cannot be replaced safely as it would affect the referential integrity of the airway network.
 * Therefore, once FAACIFP is in effect, Custom Data/CIFP/$ICAO.dat is overridden for
 * each $ICAO with PD/PE/PF records in FAACIFP.
 *
 * Hand-placed localizers
 * $X-Plane/Custom Scenery/Global Airports/Earth nav data/earth_nav.dat
 *
 * User defined layer
 * $X-Plane/Custom Data/
 * user_nav.dat
 * user_fix.dat
 */
// TODO collect errors
class XpDataCompiler
{
  Q_DECLARE_TR_FUNCTIONS(XpDataCompiler)

public:
  XpDataCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                 atools::fs::ProgressHandler *progressHandler);
  virtual ~XpDataCompiler();

  /*
   * Write a scenery entry dummy named X-Plane
   * @return true if the  process was aborted
   */
  bool writeBasepathScenery();

  /*
   * Read earth_fix.dat from either default or custom scenery depending which one exists.
   * @return true if the  process was aborted
   */
  bool compileEarthFix();

  /*
   * Read earth_awy.dat from either default or custom scenery depending which one exists.
   * @return true if the  process was aborted
   */
  bool compileEarthAirway();

  /*
   * Reads all from/to and to/from segments of all airways and creates from/via/to segments.
   */
  bool postProcessEarthAirway();

  /*
   * Read earth_awy.dat from either default or custom scenery depending which one exists.
   * @return true if the  process was aborted
   */
  bool compileEarthNav();

  /*
   * Read all CIFP/ICAO.dat airport procedure files from either default or custom scenery depending which one exists.
   * @return true if the  process was aborted
   */
  bool compileCifp();

  /*
   * Read $X-Plane/Custom Scenery/Global Airports/Earth nav data/earth_nav.dat hand placed localizers.
   * @return true if the  process was aborted
   */
  bool compileLocalizers();

  /*
   * Read $X-Plane/Custom Data/user_nav.dat
   * @return true if the  process was aborted
   */
  bool compileUserNav();

  /*
   * Read $X-Plane/Custom Data/user_fix.dat
   * @return true if the  process was aborted
   */
  bool compileUserFix();

  /*
   * Read airports from X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
   */
  bool compileDefaultApt();

  /*
   * Read airports from X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
   */
  bool compileCustomGlobalApt();

  /*
   * Read custom airports like X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
   */
  bool compileCustomApt();

  /* Close all writers and queries */
  void close();

  /* X-Plane basebath */
  const QString& getBasePath() const
  {
    return basePath;
  }

  /* Calculate number of files to be read */
  static int calculateReportCount(const atools::fs::NavDatabaseOptions& opts);

  /* minmum accepted file version */
  void setMinVersion(int value)
  {
    minVersion = value;
  }

private:
  void initQueries();
  void deInitQueries();

  /* write to metadata file table */
  void writeFile(const QString& filepath);
  void writeSceneryArea(const QString& filepath);

  /* Open file and read header */
  bool openFile(QTextStream& stream, QFile& file, const QString& filename, bool cifpFormat,
                int& lineNum, int& totalNumLines, int& fileVersion);

  /* Read file line by line and call writer for each one */
  bool readDataFile(const QString& filename, int minColumns, atools::fs::xp::XpWriter *writer,
                    atools::fs::xp::ContextFlags flags = atools::fs::xp::NO_FLAG);
  static QString buildBasePath(const NavDatabaseOptions& opts);

  /* FInd custom apt.dat like X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat */
  static QStringList findCustomAptDatFiles(const atools::fs::NavDatabaseOptions& opts);

  /* Find CIFP files like X-Plane 11/Resources/default data/CIFP/KSEA.dat */
  static QStringList findCifpFiles(const atools::fs::NavDatabaseOptions& opts);

  atools::fs::xp::ContextFlags flagsFromOptions();

  int curFileId = 0, curSceneryId = 0;
  QString basePath;
  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progress = nullptr;

  atools::sql::SqlQuery *insertFileQuery = nullptr, *insertSceneryQuery = nullptr;

  atools::fs::xp::XpFixWriter *fixWriter = nullptr;
  atools::fs::xp::XpAirwayWriter *airwayWriter = nullptr;
  atools::fs::xp::XpNavWriter *navWriter = nullptr;
  atools::fs::xp::XpAirportWriter *airportWriter = nullptr;
  atools::fs::xp::XpCifpWriter *cifpWriter = nullptr; // Procedures
  atools::fs::xp::AirwayPostProcess *airwayPostProcess = nullptr;
  atools::fs::xp::XpAirportIndex *airportIndex = nullptr;

  int minVersion = 1000;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_DATAWRITER_H
