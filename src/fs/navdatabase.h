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

#ifndef ATOOLS_FS_NAVDATABASE_H
#define ATOOLS_FS_NAVDATABASE_H

#include "logging/loggingdefs.h"

#include <QCoreApplication>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlUtil;
}

namespace fs {
class BglReaderOptions;

namespace scenery {
class SceneryCfg;
}

namespace db {
class ProgressHandler;
}

class Navdatabase
{
  Q_DECLARE_TR_FUNCTIONS(Navdatabase)

public:
  Navdatabase(const atools::fs::BglReaderOptions *readerOptions, atools::sql::SqlDatabase *sqlDb);
  void create();
  void createSchema(atools::fs::db::ProgressHandler *progress = nullptr);

  bool isAborted()
  {
    return aborted;
  }

  /* Check if scenery.cfg file exists and is valid (contains areas */
  static bool isSceneryConfigValid(const QString& filename, QString& error);

  /* Check if the base path is valid and contains a scenery directory */
  static bool isBasePathValid(const QString& filepath, QString& error);

private:
  atools::sql::SqlDatabase *db;
  const atools::fs::BglReaderOptions *options;
  bool aborted = false;
  void reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util, const QStringList& tables);

  void countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas);

  void createInternal();

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASE_H
