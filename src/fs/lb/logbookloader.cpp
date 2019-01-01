/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "fs/lb/logbookloader.h"
#include "fs/lb/logbook.h"
#include "fs/lb/logbookentryfilter.h"
#include "exception.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "settings/settings.h"

#include <QDebug>
#include <QFile>

namespace atools {
namespace fs {
namespace lb {

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

LogbookLoader::LogbookLoader(SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

void LogbookLoader::loadLogbook(const QString& filename, FsPaths::SimulatorType type,
                                const LogbookEntryFilter& filter, bool append)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    using atools::settings::Settings;

    SqlScript script(db);

    if(!append)
    {
      SqlUtil util(db);
      if(!(util.hasTable("logbook") && util.hasTable("logbook_visits")))
      {
        script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/lb/create_schema.sql"));
        db->commit();
      }
      else
      {
        script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/lb/clean_schema.sql"));
        db->commit();

        SqlQuery deleteStmt(db);
        deleteStmt.exec("delete from logbook_visits where simulator_id = " +
                        QString::number(static_cast<int>(type)));
        qInfo() << "Deleted" << deleteStmt.numRowsAffected() << "from logbook_visits of sim type" << type;

        deleteStmt.exec("delete from logbook where simulator_id = " +
                        QString::number(static_cast<int>(type)));
        qInfo() << "Deleted" << deleteStmt.numRowsAffected() << "of sim type" << type;
      }
    }

    Logbook logbook(db, type);
    logbook.read(&file, filter, append);
    numLoaded = logbook.getNumLoaded();

    if(!append)
    {
      db->commit();
      script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/lb/finish_schema.sql"));
    }

    file.close();
    db->commit();
  }
  else
  {
    db->rollback();
    throw Exception(QString("Cannot open logbook file \"%1\". Reason: %2.").
                    arg(file.fileName()).arg(file.errorString()));
  }
}

void LogbookLoader::dropDatabase()
{
  using atools::settings::Settings;

  SqlScript script(db);
  script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/drop_lb_schema.sql"));
  db->commit();
}

} // namespace lb
} // namespace fs
} // namespace atools
