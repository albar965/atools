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

#include "fs/navdatabase.h"

#include "logging/loggingdefs.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "fs/writer/datawriter.h"
#include "fs/scenery/sceneryarea.h"
#include "sql/sqlutil.h"
#include "fs/scenery/scenerycfg.h"
#include "fs/writer/routeresolver.h"

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

void Navdatabase::create()
{
  QElapsedTimer timer;
  timer.start();

  if(options->isAutocommit())
    db->setAutocommit(true);

  atools::fs::scenery::SceneryCfg cfg;
  cfg.read(options->getSceneryFile());

  SqlScript script(db);
  script.executeScript(":/atools/resources/sql/nd/drop_schema.sql");
  db->commit();

  script.executeScript(":/atools/resources/sql/nd/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_ap_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/nd/create_views.sql");
  db->commit();

  atools::fs::writer::DataWriter dataWriter(*db, *options);

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive())
      dataWriter.writeSceneryArea(area);
  db->commit();

  if(options->isResolveRoutes())
  {
    atools::fs::writer::RouteResolver resolver(*db);
    resolver.run();
    db->commit();
  }
  script.executeScript(":/atools/resources/sql/nd/update_nav_ids.sql");
  db->commit();

  script.executeScript(":/atools/resources/sql/nd/finish_schema.sql");
  db->commit();

  dataWriter.logResults();
  QDebug info(QtInfoMsg);
  atools::sql::SqlUtil(db).printTableStats(info, true);
  qInfo() << "Time" << timer.elapsed() / 1000 << "seconds";
}

} // namespace fs
} // namespace atools
