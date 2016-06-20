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

#ifndef ATOOLS_FS_DB_WRITERBASEBASIC_H
#define ATOOLS_FS_DB_WRITERBASEBASIC_H

#include "sql/sqlquery.h"

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
class BglReaderOptions;
namespace db {
class RunwayIndex;
class AirportIndex;
class DataWriter;

class WriterBaseBasic
{
public:
  WriterBaseBasic(atools::sql::SqlDatabase& sqlDb,
                  atools::fs::db::DataWriter& db,
                  const QString& tablename,
                  const QString& sqlParam);

  virtual ~WriterBaseBasic();

protected:
  atools::fs::db::DataWriter& getDataWriter()
  {
    return dataWriter;
  }

  const atools::fs::BglReaderOptions& getOptions();

  atools::fs::db::RunwayIndex *getRunwayIndex();

  atools::fs::db::AirportIndex *getAirportIndex();

  atools::sql::SqlQuery& getStmt()
  {
    return stmt;
  }

  void bind(const QString& placeholder, const QVariant& val);

  /* Boolean to integer (0 or 1) */
  void bindBool(const QString& placeholder, bool val);
  void bindNullInt(const QString& placeholder);
  void bindNullFloat(const QString& placeholder);
  void bindNullString(const QString& placeholder);

  /* Binds a null if value == 0 */
  void bindIntOrNull(const QString& placeholder, const QVariant& val);

  void executeStatement();

private:
  atools::sql::SqlQuery stmt;
  QString sql, tablename;
  atools::sql::SqlDatabase& db;
  atools::fs::db::DataWriter& dataWriter;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_WRITERBASEBASIC_H
