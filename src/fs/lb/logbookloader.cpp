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

#include "fs/lb/logbookloader.h"
#include "fs/lb/logbook.h"
#include "fs/lb/logbookentryfilter.h"
#include "exception.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "settings/settings.h"

#include <QFile>

namespace atools {
namespace fs {
namespace lb {

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;

LogbookLoader::LogbookLoader(SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

void LogbookLoader::loadLogbook(const QString& filename, const LogbookEntryFilter& filter, bool append)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    using atools::settings::Settings;

    SqlScript script(db);

    if(!append)
    {
      script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/create_lb_schema.sql"));
      db->commit();
    }

    Logbook logbook(db);
    logbook.read(&file, filter, append);
    numLoaded = logbook.getNumLoaded();
    db->commit();

    if(!append)
    {
      script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/finish_lb_schema.sql"));
      db->commit();
    }

    file.close();
  }
  else
    throw Exception(QString("Cannot open logbook file \"%1\". Reason: %2.").
                    arg(file.fileName()).arg(file.errorString()));
}

} // namespace lb
} // namespace fs
} // namespace atools
