/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "sql/sqlexception.h"

namespace atools {

namespace sql {

SqlException::SqlException(const QSqlError& sqlError, const QString& message, const QString& message2)
  : Exception(message)
{
  this->sqlErr = sqlError;
  this->msg2 = message2;
  createSqlMessage();
}

SqlException::SqlException(const QString& message, const QString& message2)
  : Exception(message)
{
  this->msg2 = message2;
  createSqlMessage();
}

SqlException::~SqlException()
{
}

void SqlException::createSqlMessage()
{
  QString msg;
  if(sqlErr.isValid())
  {
    msg = sqlErr.text();
    if(!sqlErr.text().endsWith("."))
      msg += ".";
  }

  if(!message.isEmpty())
  {
    if(!msg.isEmpty())
      msg += " ";
    msg += message;
    if(!msg.endsWith("."))
      msg += ".";
  }

  if(!msg2.isEmpty())
  {
    if(!msg.isEmpty())
      msg += " ";
    msg += msg2;
    if(!msg.endsWith("."))
      msg += ".";
  }

  whatMessage = msg.toLocal8Bit();
}

} // namespace sql

} // namespace atools
