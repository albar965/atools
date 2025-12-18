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

#ifndef ATOOLS_SQL_SQLEXCEPTION_H
#define ATOOLS_SQL_SQLEXCEPTION_H

#include "exception.h"

#include <QCoreApplication>

class QSqlError;
namespace atools {
namespace sql {

class SqlRecord;
class SqlQuery;
class SqlDatabase;

/*
 * Exception for SQL module which decodes error codes from SQL classes.
 *
 * Qt Concurrent supports throwing and catching exceptions across thread boundaries,
 * provided that the exception inherit from QException and implement two helper functions.
 */
class SqlException :
  public atools::Exception
{
  Q_DECLARE_TR_FUNCTIONS(atools::Exception)

public:
  explicit SqlException()
  {
  }

  explicit SqlException(const QString& message);
  explicit SqlException(const atools::sql::SqlDatabase *db, const QString& message = QString());
  explicit SqlException(const atools::sql::SqlQuery *query, const QString& message = QString());
  explicit SqlException(const atools::sql::SqlRecord *record, const QString& message = QString());

  /* Override from QException to allow passing the exception across threads */
  virtual void raise() const override;
  virtual SqlException *clone() const override;

private:
  void messageDb(QStringList& msgList, const SqlDatabase *db);
  void messageQuery(QStringList& msgList, const SqlQuery *query);
  void messageRecord(QStringList& msgList, const SqlRecord *record);
  void messageErr(QStringList& msgList, const QSqlError& err);

};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLEXCEPTION_H
