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

#include <QElapsedTimer>

#include "fs/scenery/scenerycfg.h"

namespace atools {
namespace fs {

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

Navdatabase::Navdatabase(const atools::fs::BglReaderOptions& readerOptions, sql::SqlDatabase *sqlDb)
  : db(sqlDb), options(readerOptions)
{

}

void Navdatabase::create()
{
  QElapsedTimer timer;
  timer.start();

  atools::fs::scenery::SceneryCfg cfg;
  cfg.read(options.getSceneryFile());

  SqlScript script(db);
  script.executeScript(":/atools/resources/sql/writer/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/writer/create_ap_schema.sql");
  script.executeScript(":/atools/resources/sql/writer/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/writer/create_views.sql");
  db->commit();

  SqlQuery(db).exec("PRAGMA foreign_keys = ON");
  db->commit();

  atools::fs::writer::DataWriter dataWriter(*db, options);

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    if(area.isActive())
      dataWriter.writeSceneryArea(area);
  db->commit();

  // atools::fs::writer::RouteResolver resolver(db);
  // resolver.run();
  // db.commit();

  script.executeScript(":/atools/resources/sql/writer/finish_schema.sql");
  db->commit();

  dataWriter.logResults();
  QDebug info(QtInfoMsg);
  atools::sql::SqlUtil(db).printTableStats(info, true);
  qInfo() << "Time" << timer.elapsed() / 1000 << "seconds";
}

} // namespace fs
} // namespace atools
