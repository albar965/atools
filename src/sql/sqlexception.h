/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include <QSqlError>

namespace atools {
namespace sql {

/*
 * Exception for SQL module which also carries the QSqlError
 */
class SqlException :
  public atools::Exception
{
public:
  SqlException(const QSqlError& sqlErr, const QString& message = QString(), const QString& message2 = QString());
  SqlException(const QString& message = QString(), const QString& message2 = QString());

  virtual ~SqlException();

  QString getMessage2() const
  {
    return msg2;
  }

  QSqlError getSqlError() const
  {
    return sqlErr;
  }

private:
  void createSqlMessage();

  QSqlError sqlErr;
  QString msg2;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLEXCEPTION_H
