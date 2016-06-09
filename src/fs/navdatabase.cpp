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
#include "fs/db/airwayresolver.h"
#include "fs/db/routeedgewriter.h"
#include "fs/db/progresshandler.h"
#include "fs/scenery/fileresolver.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>

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

void Navdatabase::create()
{
  createInternal();

  if(aborted)
  {
    db->rollback();
    // Create an empty schema to avoid application crashes
    createSchema();
  }
}

void Navdatabase::createSchema(db::ProgressHandler *progress)
{
  SqlScript script(db, true /* options->isVerbose()*/);

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Views"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_view.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Routing and Search"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_routing_search.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Navigation Aids"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_nav.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airport Facilites"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_airport_facilities.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Approaches"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_approach.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airports"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_airport.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Metadata"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/drop_meta.sql");

  db->commit();

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Creating Schema"))) == true)
      return;

  script.executeScript(":/atools/resources/sql/nd/create_boundary_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_ap_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_route_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_views.sql");
  db->commit();
}

bool Navdatabase::isSceneryConfigValid(const QString& filename, QString& error)
{
  QFileInfo fi(filename);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isFile())
      {
        try
        {
          atools::fs::scenery::SceneryCfg cfg;
          cfg.read(filename);
          return !cfg.getAreas().isEmpty();
        }
        catch(atools::Exception& e)
        {
          qWarning() << "Caught exception reading" << filename << ":" << e.what();
          error = e.what();
        }
        catch(...)
        {
          qWarning() << "Caught unknown exception reading" << filename;
          error = "Unknown exception while reading file";
        }
      }
      else
        error = tr("File is not a regular file");
    }
    else
      error = tr("File is not readable");
  }
  else
    error = tr("File does not exist");
  return false;
}

bool Navdatabase::isBasePathValid(const QString& filepath, QString& error)
{
  QFileInfo fi(filepath);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isDir())
      {
        // If path exists check for scenery directory
        QDir dir(filepath);
        QFileInfoList scenery = dir.entryInfoList({"scenery"}, QDir::Dirs);
        if(!scenery.isEmpty())
          return true;
        else
          error = tr("Does not contain a \"Scenery\" directory");
      }
      else
        error = tr("Is not a directory");
    }
    else
      error = tr("Directory is not readable");
  }
  else
    error = tr("Directory does not exist");
  return false;
}

// Number of progress steps besides scenery areas
const int NUM_STEPS = 18;
const int NUM_DB_REPORT_STEPS = 4;
const int NUM_RESOLVE_AIRWAY_STEPS = 1;

void Navdatabase::createInternal()
{
  QElapsedTimer timer;
  timer.start();

  if(options->isAutocommit())
    db->setAutocommit(true);

  atools::fs::scenery::SceneryCfg cfg;
  cfg.read(options->getSceneryFile());

  int numFiles = 0, numSceneryAreas = 0;

  countFiles(cfg, &numFiles, &numSceneryAreas);

  db::ProgressHandler progress(options);

  int total = numFiles + numSceneryAreas + NUM_STEPS;

  if(options->isDatabaseReport())
    total += NUM_DB_REPORT_STEPS;

  if(options->isResolveAirways())
    total += NUM_RESOLVE_AIRWAY_STEPS;

  int numRouteSteps = total / 4;
  if(options->isCreateRouteTables())
    total += numRouteSteps;

  progress.setTotal(total);

  createSchema(&progress);
  if(aborted)
    return;

  atools::fs::db::DataWriter dataWriter(*db, *options, &progress);

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive() && options->includePath(area.getLocalPath()))
    {
      if((aborted = progress.report(&area)) == true)
        return;

      dataWriter.writeSceneryArea(area);
      if((aborted = dataWriter.isAborted()) == true)
        return;
    }
  db->commit();

  if((aborted = progress.reportOther(tr("Creating Indexes"))) == true)
    return;

  SqlScript script(db, true /*options->isVerbose()*/);
  script.executeScript(":/atools/resources/sql/nd/create_indexes_post_load.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Clean up"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/delete_duplicates.sql");
  db->commit();

  if(options->isResolveAirways())
  {
    if((aborted = progress.reportOther(tr("Creating airways"))) == true)
      return;

    atools::fs::db::AirwayResolver resolver(db, progress);

    if((aborted = resolver.run()) == true)
      return;

    db->commit();
  }

  if((aborted = progress.reportOther(tr("Updating Waypoint ids"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/update_wp_ids.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Updating Navigation ids"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/update_nav_ids.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Updating ILS ids"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/update_ils_ids.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Populating Nav Search Table"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/populate_nav_search.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Populating Route Node Table"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/populate_route_node.sql");
  db->commit();

  if(options->isCreateRouteTables())
  {
    if((aborted = progress.reportOther(tr("Populating Route Edge Table for VOR and NDB"))) == true)
      return;

    atools::fs::db::RouteEdgeWriter edgeWriter(db, progress, numRouteSteps);
    if((aborted = edgeWriter.run()) == true)
      return;

    db->commit();
  }

  if((aborted = progress.reportOther(tr("Populating Route Edge Table for Waypoints"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/populate_route_edge.sql");
  db->commit();

  if((aborted = progress.reportOther(tr("Creating Search Indexes"))) == true)
    return;

  script.executeScript(":/atools/resources/sql/nd/finish_schema.sql");
  db->commit();

  if(options->isDatabaseReport())
  {
    // Do a report of problems rather than failing totally during loading
    dataWriter.logResults();
    QDebug info(QtInfoMsg);
    atools::sql::SqlUtil util(db);

    if((aborted = progress.reportOther(tr("Creating table statistics"))) == true)
      return;

    qDebug() << "printTableStats";
    info << endl;
    util.printTableStats(info);

    if((aborted = progress.reportOther(tr("Creating report on values"))) == true)
      return;

    qDebug() << "createColumnReport";
    info << endl;
    util.createColumnReport(info);

    if((aborted = progress.reportOther(tr("Creating report on duplicates"))) == true)
      return;

    info << endl;
    qDebug() << "reportDuplicates airport";
    util.reportDuplicates(info, "airport", "airport_id", {"ident"});
    info << endl;
    qDebug() << "reportDuplicates vor";
    util.reportDuplicates(info, "vor", "vor_id", {"ident", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates ndb";
    util.reportDuplicates(info, "ndb", "ndb_id", {"ident", "type", "frequency", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates waypoint";
    util.reportDuplicates(info, "waypoint", "waypoint_id", {"ident", "type", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates ils";
    util.reportDuplicates(info, "ils", "ils_id", {"ident", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates marker";
    util.reportDuplicates(info, "marker", "marker_id", {"type", "heading", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates helipad";
    util.reportDuplicates(info, "helipad", "helipad_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates parking";
    util.reportDuplicates(info, "parking", "parking_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates start";
    util.reportDuplicates(info, "start", "start_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates runway";
    util.reportDuplicates(info, "runway", "runway_id", {"heading", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates bgl_file";
    util.reportDuplicates(info, "bgl_file", "bgl_file_id", {"filename"});
    info << endl;

    if((aborted = progress.reportOther(tr("Creating report on coordinate duplicates"))) == true)
      return;

    reportCoordinateViolations(info, util, {"airport", "vor", "ndb", "marker", "waypoint"});
  }
  progress.reportFinish();

  qDebug() << "Time" << timer.elapsed() / 1000 << "seconds";
}

void Navdatabase::reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util,
                                             const QStringList& tables)
{
  for(QString table : tables)
  {
    qDebug() << "reportCoordinateViolations" << table;
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "lonx", -180.f, 180.f);
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "laty", -90.f, 90.f);
  }
}

void Navdatabase::countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas)
{
  qDebug() << "Counting files";

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive() && options->includePath(area.getLocalPath()))
    {
      atools::fs::scenery::FileResolver resolver(*options, true);
      *numFiles += resolver.getFiles(area);
      (*numSceneryAreas)++;
    }
  qDebug() << "Counting files done." << numFiles << "files to process";
}

} // namespace fs
} // namespace atools
