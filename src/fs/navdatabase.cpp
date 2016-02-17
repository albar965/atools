/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "logging/loggingdefs.h"
#include "fs/navdatabase.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "fs/db/datawriter.h"
#include "fs/scenery/sceneryarea.h"
#include "sql/sqlutil.h"
#include "fs/scenery/scenerycfg.h"
#include "fs/db/routeresolver.h"
#include "fs/db/progresshandler.h"
#include "fs/scenery/fileresolver.h"

#include <QElapsedTimer>

namespace atools {
namespace fs {

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

Navdatabase::Navdatabase(const BglReaderOptions *readerOptions, sql::SqlDatabase *sqlDb)
  : db(sqlDb), options(readerOptions)
{

}

void Navdatabase::countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas)
{
  qInfo() << "Counting files";

  QStringList files;
  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive() && options->includePath(area.getLocalPath()))
    {
      atools::fs::scenery::FileResolver resolver(*options, true);
      resolver.getFiles(area, files);
      (*numSceneryAreas)++;
      *numFiles += files.size();
      files.clear();
    }
  qInfo() << "Counting files done." << numFiles << "file to process";
}

void Navdatabase::create()
{
  QElapsedTimer timer;
  timer.start();

  if(options->isAutocommit())
    db->setAutocommit(true);

  atools::fs::scenery::SceneryCfg cfg;
  cfg.read(options->getSceneryFile());

  int numFiles = 0, numSceneryAreas = 0;

  countFiles(cfg, &numFiles, &numSceneryAreas);

  db::ProgressHandler progressHandler(options);

  int total = numFiles + numSceneryAreas + 6;
  if(options->isDatabaseReport())
    total += 4;
  if(options->isResolveRoutes())
    total += 1;

  progressHandler.setTotal(total);

  SqlScript script(db, options->isVerbose());
  progressHandler.reportProgressOther(QObject::tr("Dropping old database schema"));
  script.executeScript(":/atools/resources/sql/nd/drop_schema.sql");
  db->commit();

  progressHandler.reportProgressOther(QObject::tr("Creating new database schema"));
  script.executeScript(":/atools/resources/sql/nd/create_boundary_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_ap_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_views.sql");
  db->commit();

  atools::fs::db::DataWriter dataWriter(*db, *options, &progressHandler);

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive() && options->includePath(area.getLocalPath()))
    {
      progressHandler.reportProgress(&area);
      dataWriter.writeSceneryArea(area);
    }
  db->commit();

  if(options->isResolveRoutes())
  {
    progressHandler.reportProgressOther(QObject::tr("Creating routes"));
    atools::fs::db::RouteResolver resolver(*db);
    resolver.run();
    db->commit();
  }

  progressHandler.reportProgressOther(QObject::tr("Creating post load indexes"));
  script.executeScript(":/atools/resources/sql/nd/create_indexes_post_load.sql");
  db->commit();

  progressHandler.reportProgressOther(QObject::tr("Removing duplicates"));
  script.executeScript(":/atools/resources/sql/nd/delete_duplicates.sql");
  db->commit();

  progressHandler.reportProgressOther(QObject::tr("Updating navigation ids"));
  script.executeScript(":/atools/resources/sql/nd/update_nav_ids.sql");
  db->commit();

  progressHandler.reportProgressOther(QObject::tr("Creating final indexes"));
  script.executeScript(":/atools/resources/sql/nd/finish_schema.sql");
  db->commit();

  if(options->isDatabaseReport())
  {
    // Do a report of problems rather than failing totally during loading
    dataWriter.logResults();
    QDebug info(QtInfoMsg);
    atools::sql::SqlUtil util(db);

    progressHandler.reportProgressOther(QObject::tr("Creating table statistics"));
    info << endl;
    util.printTableStats(info);

    progressHandler.reportProgressOther(QObject::tr("Creating report on values"));
    info << endl;
    util.createColumnReport(info);

    progressHandler.reportProgressOther(QObject::tr("Creating report on duplicates"));
    info << endl;
    util.reportDuplicates(info, "airport", "airport_id", {"ident"});
    info << endl;
    util.reportDuplicates(info, "vor", "vor_id", {"ident", "region", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "ndb", "ndb_id", {"ident", "type", "frequency", "region", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "waypoint", "waypoint_id", {"ident", "type", "region", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "ils", "ils_id", {"ident", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "marker", "marker_id", {"type", "heading", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "helipad", "helipad_id", {"lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "parking", "parking_id", {"lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "start", "start_id", {"lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "runway", "runway_id", {"heading", "lonx", "laty"});
    info << endl;
    util.reportDuplicates(info, "bgl_file", "bgl_file_id", {"filename"});
    info << endl;

    progressHandler.reportProgressOther(QObject::tr("Creating report on coordinate duplicates"));
    reportCoordinateViolations(info, util, {"airport", "vor", "ndb", "marker", "waypoint"});
  }

  qInfo() << "Time" << timer.elapsed() / 1000 << "seconds";
}

void Navdatabase::reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util,
                                             const QStringList& tables)
{
  for(QString table : tables)
  {
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "lonx", -180.f, 180.f);
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "laty", -90.f, 90.f);
  }
}

} // namespace fs
} // namespace atools
