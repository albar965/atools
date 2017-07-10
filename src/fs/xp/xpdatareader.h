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
class XpWriter;
class AirwayPostProcess;

/*
 * Provides methods to read X-Plane data from text files into the database.
 */
class XpDataCompiler
{
  Q_DECLARE_TR_FUNCTIONS(XpDataCompiler)

public:
  XpDataCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                 atools::fs::ProgressHandler *progress);
  virtual ~XpDataCompiler();

  bool writeBasepathScenery();
  bool compileEarthFix();
  bool compileEarthAirway();
  bool postProcessEarthAirway();
  bool compileEarthNav();
  bool writeCifp();

  bool writeLocalizers();

  bool writeUserNav();
  bool writeUserFix();

  /* Close all writers and queries */
  void close();

  const QString& getBasePath() const
  {
    return basePath;
  }

  static int calculateFileCount(const atools::fs::NavDatabaseOptions& opts);

  void setMinVersion(int value)
  {
    minVersion = value;
  }

  bool compileDefaultApt();
  bool compileCustomGlobalApt();
  bool compileCustomApt();

private:
  void initQueries();
  void deInitQueries();
  void writeFile(const QString& filepath);
  void writeSceneryArea(const QString& filepath);
  bool openFile(QTextStream& stream, QFile& file, const QString& filename);
  bool readDataFile(const QString& filename, int minColumns, atools::fs::xp::XpWriter *writer);
  static QString buildBasePath(const NavDatabaseOptions& opts);

  int curFileId = 0, curSceneryId = 0;
  QString basePath;
  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progressHandler = nullptr;

  atools::sql::SqlQuery *insertFileQuery = nullptr, *insertSceneryQuery = nullptr;

  atools::fs::xp::XpFixWriter *fixWriter = nullptr;
  atools::fs::xp::XpAirwayWriter *airwayWriter = nullptr;
  atools::fs::xp::XpNavWriter *navWriter = nullptr;
  atools::fs::xp::XpAirportWriter *airportWriter = nullptr;
  atools::fs::xp::AirwayPostProcess *airwayPostProcess = nullptr;

  int minVersion = 1100;
  // Base layer
  // $X-Plane/Resources/default data/
  // earth_fix.dat
  // earth_awy.dat
  // earth_nav.dat
  // CIFP/$ICAO.dat (where $ICAO is each airport with instrument procedures)

  // Updated base
  // If these files are present, the X-Plane base layer is ignored.
  // $X-Plane/Custom Data/
  // earth_fix.dat
  // earth_awy.dat
  // earth_nav.dat
  // CIFP/$ICAO.dat (where $ICAO is each airport with instrument procedures)

  // In X-Plane 11, this file is used to replace P* records with the latest from the FAA.
  // $X-Plane/Custom Data/
  // FAACIFP18
  // Note that no enroute waypoints, VHF enroute navaids, or enroute airways are loaded from this file.
  // These cannot be replaced safely as it would affect the referential integrity of the airway network.
  // Therefore, once FAACIFP is in effect, Custom Data/CIFP/$ICAO.dat is overridden for
  // each $ICAO with PD/PE/PF records in FAACIFP.

  // Hand-placed localizers
  // $X-Plane/Custom Scenery/Global Airports/Earth nav data/earth_nav.dat

  // User defined layer
  // $X-Plane/Custom Data/
  // user_nav.dat
  // user_fix.dat

  static QStringList findCustomAptDatFiles(const atools::fs::NavDatabaseOptions& opts);
  static QStringList findCifpFiles(const atools::fs::NavDatabaseOptions& opts);

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_DATAWRITER_H
