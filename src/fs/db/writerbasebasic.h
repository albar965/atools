/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
#include "fs/bgl/bglposition.h"

#include <QDataStream>
#include <QIODevice>

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
class NavDatabaseOptions;
namespace db {
class RunwayIndex;
class DataWriter;

/*
 * Template free base class for all writer classes that store BGL record content into the database.
 * Keeps the SQL statement and has utilitly methods for binding values to it.
 */
class WriterBaseBasic
{
public:
  /*
   * @param sqlDb an open database
   * @param dw datawriter as parent that keeps all writers
   * @param tablename table to insert content. An prepared insert statement including all columns
   *        will be generated from this
   * @param sqlParam custom insert statement to insert data. If this is set tablename will be ignored.
   */
  WriterBaseBasic(atools::sql::SqlDatabase& sqlDb,
                  atools::fs::db::DataWriter& dw,
                  const QString& tablename,
                  const QString& sqlParam);

  virtual ~WriterBaseBasic();

protected:
  atools::fs::db::DataWriter& getDataWriter()
  {
    return dataWriter;
  }

  const atools::fs::NavDatabaseOptions& getOptions();
  atools::fs::db::RunwayIndex *getRunwayIndex();

  /*
   * Bind a value to the insert statement
   * @param placeholder
   * @param val
   */
  void bind(const QString& placeholder, const QVariant& val);

  /* Boolean to integer (0 or 1) */
  void bindBool(const QString& placeholder, bool val);

  /* Bind type specific null values */
  void bindNullInt(const QString& placeholder);
  void bindNullFloat(const QString& placeholder);
  void bindNullString(const QString& placeholder);

  /* Binds a null if value == 0 */
  void bindIntOrNull(const QString& placeholder, const QVariant& val);

  /* Binds a null if string val is empty */
  void bindStrOrNull(const QString& placeholder, const QString& val);

  /* Bin a list of numbers in a byte array */
  template<typename TYPE>
  void bindNumberList(const QString& placeholder, const QList<TYPE>& list);

  /* Execute the insert and throw an exception if nothing was inserted */
  void executeStatement();

private:
  atools::sql::SqlQuery sqlQuery; // Either custom query or generated insert statement
  QString sqlStatement, tablename;
  atools::sql::SqlDatabase& db;
  atools::fs::db::DataWriter& dataWriter;

};

template<typename TYPE>
void WriterBaseBasic::bindNumberList(const QString& placeholder, const QList<TYPE>& list)
{
  QByteArray blob;
  QDataStream out(&blob, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << static_cast<quint32>(list.size());
  for(const TYPE& i : list)
    out << i;

  bind(placeholder, blob);
}

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_WRITERBASEBASIC_H
