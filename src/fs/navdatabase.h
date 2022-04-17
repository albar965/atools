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

#ifndef ATOOLS_FS_NAVDATABASE_H
#define ATOOLS_FS_NAVDATABASE_H

#include "fs/fspaths.h"
#include "fs/navdatabaseflags.h"

#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlUtil;
}

namespace fs {
class NavDatabaseOptions;
class NavDatabaseErrors;

namespace scenery {
class SceneryCfg;
class AddOnComponent;
class SceneryArea;
class ManifestJson;
}

namespace db {
class DataWriter;
}

namespace xp {
class XpDataCompiler;
}

namespace ng {
class DfdCompiler;
}

class ProgressHandler;

/*
 * Class for creating the full navigation database. Reads scenery.cfg, reads BGL files of each found
 * scenery area and writes all data to the database tables.
 * Configuration and progress callback is done with class atools::fs::BglReaderOptions.
 */
class NavDatabase
{
  Q_DECLARE_TR_FUNCTIONS(Navdatabase)

public:
  /*
   * Creates the object but does not load anything yet.
   * @param readerOptions Configuration and progress callback options. Optional for schema creation
   * @param sqlDb Database to fill with data
   */
  NavDatabase(const atools::fs::NavDatabaseOptions *readerOptions, atools::sql::SqlDatabase *sqlDb,
              atools::fs::NavDatabaseErrors *databaseErrors, const QString& revision);

  /* Read all BGL or X-Plane files and load data into database. atools::Exception is thrown in case of error.
   * @param codec Scenery.cfg codec only applies to FSX/P3D */
  atools::fs::ResultFlags create(const QString& codec);

  /* Does not load anything and only creates the empty database schema.
   * Configuration is not used and can be null. atools::Exception is thrown in case of error.
   * Opens own transaction and commits if successfull */
  void createSchema();

  /* Creates only metadata and boundary tables for user airspaces. Does not use progress and does not open a
   * transaction. */
  void createAirspaceSchema();

  /*
   * Checks if scenery.cfg file exists and is valid (contains areas).
   * Only FSX and P3D.
   *
   * @param filename Scenery.cfg filename
   * @param codec Scenery.cfg codec
   * @param error An error message will be placed in this string
   * @return true if file is valid
   */
  static bool isSceneryConfigValid(const QString& filename, const QString& codec, QStringList& errors);

  /*
   * Checks if the flight simulator base path is valid and contains a "scenery" directory.
   *
   * @param filepath path to flight simulator base (the directory containing e.g. fsx.exe)
   * @param error An error message will be placed in this string
   * @return true if path is valid
   */
  static bool isBasePathValid(const QString& filepath, QStringList& errors, atools::fs::FsPaths::SimulatorType type);

  /* Executes all statements like create index in the table script and deletes it afterwards */
  static void runPreparationScript(atools::sql::SqlDatabase& db);

  /* Delete all tables that are not used in versions > 2.4.5 */
  static void runPreparationPost245(atools::sql::SqlDatabase& db);

private:
  /* Creates database schema only */
  void createSchemaInternal(atools::fs::ProgressHandler *progress = nullptr);

  /* Internal creation of the full database */
  atools::fs::ResultFlags createInternal(const QString& sceneryConfigCodec);

  /* Read FSX/P3D scenery configuration */
  void readSceneryConfigFsxP3d(atools::fs::scenery::SceneryCfg& cfg);

  /* Fill MSFS scenery configuration with the two default entries */
  void readSceneryConfigMsfs(scenery::SceneryCfg& cfg);

  /* Source dependent compilation methods */

  /* FSX, FSX SE and P3D */
  bool loadFsxP3d(atools::fs::ProgressHandler *progress, atools::fs::db::DataWriter *fsDataWriter,
                  const scenery::SceneryCfg& cfg);

  /* MSFS 2020 */
  bool loadMsfs(atools::fs::ProgressHandler *progress, atools::fs::db::DataWriter *fsDataWriter,
                const scenery::SceneryCfg& cfg);

  /* X-Plane */
  bool loadXplane(atools::fs::ProgressHandler *progress, atools::fs::xp::XpDataCompiler *xpDataCompiler,
                  const atools::fs::scenery::SceneryArea& area);

  bool loadFsxP3dMsfsPost(ProgressHandler *progress);

  /* Navigraph / DFD */
  bool loadDfd(atools::fs::ProgressHandler *progress, atools::fs::ng::DfdCompiler *dfdCompiler,
               const atools::fs::scenery::SceneryArea& area);

  bool loadFsxP3dMsfsSimulator(ProgressHandler *progress, db::DataWriter *fsDataWriter,
                               const QList<atools::fs::scenery::SceneryArea>& areas);

  /* Reporting to log file and/or console */
  bool createDatabaseReport(ProgressHandler *progress);

  /* Print row counts to log file */
  void createDatabaseReportShort();

  bool basicValidation(ProgressHandler *progress, bool& foundBasicValidationError);
  void basicValidateTable(const QString& table, int minCount, bool& foundBasicValidationError);
  void reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util, const QStringList& tables);

  /* Count files in FSX/P3D scenery configuration */
  void countFiles(ProgressHandler *progress, const QList<scenery::SceneryArea>& areas, int& numFiles,
                  int& numSceneryAreas);
  int nextAreaNum(const QList<atools::fs::scenery::SceneryArea>& areas);

  /* Run and report SQL script */
  bool runScript(atools::fs::ProgressHandler *progress, const QString& scriptFile, const QString& message);

  void createPreparationScript();
  void dropAllIndexes();

  void readAddOnComponents(int& areaNum, atools::fs::scenery::SceneryCfg& cfg,
                           QVector<scenery::AddOnComponent>& noLayerComponents,
                           QStringList& noLayerPaths, QSet<QString>& addonPaths, const QFileInfo& addonEntry);
  QFileInfo buildAddonFile(const QFileInfo& addonEntry);

  /* Count the number of total steps for progress max value. This covers all files and other steps. */
  int countDfdSteps();
  int countXplaneSteps(ProgressHandler *progress);
  int countFsxP3dSteps(ProgressHandler *progress, const scenery::SceneryCfg& cfg);
  int countMsfsSteps(ProgressHandler *progress, const scenery::SceneryCfg& cfg);
  int countMsSimSteps();

  /* Detect Navigraph navdata update packages for special handling */
  bool checkNavigraphNavdataUpdate(atools::fs::scenery::ManifestJson& manifest);

  /* Detect Navigraph maintenance package for exclusion. true if should be excluded */
  bool checkNavigraphNavdataExclude(atools::fs::scenery::ManifestJson& manifest);

  /* For metadata */

  atools::sql::SqlDatabase *db;
  atools::fs::NavDatabaseErrors *errors = nullptr;
  const atools::fs::NavDatabaseOptions *options = nullptr;
  bool aborted = false;
  QString gitRevision;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASE_H
