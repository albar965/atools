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

#include <QString>

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

class FixWriter;
class NavWriter;
class AirwayWriter;
class Writer;

class XplaneDataCompiler
{
public:
  XplaneDataCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                     atools::fs::ProgressHandler *progress);
  virtual ~XplaneDataCompiler();

  bool compileMeta();
  bool compileEarthFix();
  bool compileEarthAirway();
  bool compileEarthNav();
  bool compileApt();
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

private:
  void initQueries();
  void deInitQueries();
  void readFile(const QString& filepath);
  void writeSceneryArea(const QString& filepath);
  bool openFile(QTextStream& stream, QFile& file, const QString& filename);
  bool readDatFile(const QString& filename, int minColumns, atools::fs::xp::Writer *writer);
  static QString buildBasePath(const NavDatabaseOptions& opts);

  int curFileId = 0, curSceneryId = 0;
  QString basePath;
  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progressHandler = nullptr;

  atools::sql::SqlQuery *insertFileQuery = nullptr, *insertSceneryQuery = nullptr;

  atools::fs::xp::FixWriter *fixWriter = nullptr;
  atools::fs::xp::AirwayWriter *airwayWriter = nullptr;
  atools::fs::xp::NavWriter *navWriter = nullptr;

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

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_DATAWRITER_H
