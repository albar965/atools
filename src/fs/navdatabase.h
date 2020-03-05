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

#include "fs/navdatabaseerrors.h"

#include "fs/fspaths.h"

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

namespace scenery {
class SceneryCfg;
class AddOnComponent;
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

  /* Read all BGL files and load data into database. atools::Exception is thrown in case of error.
   * @param codec Scenery.cfg codec */
  void create(const QString& codec);

  /* Does not load anything and only creates the empty database schema.
   * Configuration is not used and can be null. atools::Exception is thrown in case of error.
   * Opens own transaction and commits if successfull */
  void createSchema();

  /* Creates only metadata and boundary tables for user airspaces. Does not use progress and does not open a
   * transaction. */
  void createAirspaceSchema();

  /*
   *
   * @return true if loading was aborted by the progress callback
   */
  bool isAborted()
  {
    return aborted;
  }

  /*
   * Checks if scenery.cfg file exists and is valid (contains areas).
   *
   * @param filename Scenery.cfg filename
   * @param codec Scenery.cfg codec
   * @param error An error message will be placed in this string
   * @return true if file is valid
   */
  static bool isSceneryConfigValid(const QString& filename, const QString& codec, QString& error);

  /*
   * Checks if the flight simulator base path is valid and contains a "scenery" directory.
   *
   * @param filepath path to flight simulator base (the directory containing e.g. fsx.exe)
   * @param error An error message will be placed in this string
   * @return true if path is valid
   */
  static bool isBasePathValid(const QString& filepath, QString& error, atools::fs::FsPaths::SimulatorType type);

  /* Executes all statements like create index in the table script and deletes it afterwards */
  static void runPreparationScript(atools::sql::SqlDatabase& db);

private:
  /* Creates database schema only */
  void createSchemaInternal(atools::fs::ProgressHandler *progress = nullptr);

  /* Internal creation of the full database */
  void createInternal(const QString& sceneryConfigCodec);

  /* Read FSX/P3D scenery configuration */
  void readSceneryConfig(atools::fs::scenery::SceneryCfg& cfg);

  /* Source dependent compilation methods */
  bool loadFsxP3d(atools::fs::ProgressHandler *progress, atools::fs::db::DataWriter *fsDataWriter,
                  const scenery::SceneryCfg& cfg);
  bool loadXplane(atools::fs::ProgressHandler *progress, atools::fs::xp::XpDataCompiler *xpDataCompiler,
                  const atools::fs::scenery::SceneryArea& area);
  bool loadDfd(atools::fs::ProgressHandler *progress, atools::fs::ng::DfdCompiler *dfdCompiler,
               const atools::fs::scenery::SceneryArea& area);

  /* Reporting to log file and/or console */
  bool createDatabaseReport(ProgressHandler *progress);
  bool basicValidation(ProgressHandler *progress);
  void basicValidateTable(const QString& table, int minCount);
  void reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util, const QStringList& tables);

  /* Count files in FSX/P3D scenery configuration */
  void countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas);

  /* Run and report SQL script */
  bool runScript(atools::fs::ProgressHandler *progress, const QString& scriptFile, const QString& message);

  void createPreparationScript();
  void dropAllIndexes();

  void readAddOnComponents(int& areaNum, atools::fs::scenery::SceneryCfg& cfg,
                           QVector<scenery::AddOnComponent>& noLayerComponents,
                           QStringList& noLayerPaths, QSet<QString>& addonPaths, const QFileInfo& addonEntry);
  QFileInfo buildAddonFile(const QFileInfo& addonEntry);

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
