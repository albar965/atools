/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "sql/sqlrecord.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlField>
#include <QVariant>
#include <QStringBuilder>

namespace atools {
namespace sql {

QVariant SqlRecord::value(int i) const
{
  QVariant retval = sqlRecord.value(i);
  if(!retval.isValid())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Value index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");
  return retval;
}

QVariant SqlRecord::value(const QString& name) const
{
  QVariant retval = sqlRecord.value(name);
  if(!retval.isValid())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Value name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  return retval;

}

QDateTime SqlRecord::valueDateTime(int i) const
{
  return value(i).toDateTime();
}

QDateTime SqlRecord::valueDateTime(const QString& name) const
{
  return value(name).toDateTime();
}

QDateTime SqlRecord::valueDateTime(const QString& name, const QDateTime& defaultValue) const
{
  return contains(name) ? valueDateTime(name) : defaultValue;
}

bool SqlRecord::isNull(int i) const
{
  if(sqlRecord.fieldName(i).isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");

  return sqlRecord.isNull(i);
}

bool SqlRecord::isNull(const QString& name) const
{
  if(sqlRecord.indexOf(name) == -1)
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  return sqlRecord.isNull(name);
}

int SqlRecord::indexOf(const QString& name) const
{
  int retval = sqlRecord.indexOf(name);
  if(retval == -1)
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  return retval;
}

QString SqlRecord::fieldName(int i) const
{
  QString retval = sqlRecord.fieldName(i);
  if(retval.isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");
  return retval;
}

QVariant::Type SqlRecord::fieldType(int i) const
{
  QSqlField retval = sqlRecord.field(i);
  if(sqlRecord.fieldName(i).isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");
  return retval.type();
}

QVariant::Type SqlRecord::fieldType(const QString& name) const
{
  QSqlField retval = sqlRecord.field(name);
  if(!contains(name))
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");

  return retval.type();
}

bool SqlRecord::isGenerated(int i) const
{
  if(sqlRecord.fieldName(i).isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");

  return sqlRecord.isGenerated(i);
}

bool SqlRecord::isGenerated(const QString& name) const
{
  if(!contains(name))
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  return sqlRecord.isGenerated(name);
}

bool SqlRecord::isEmpty() const
{
  return sqlRecord.isEmpty();
}

bool SqlRecord::contains(const QString& name) const
{
  return sqlRecord.contains(name);
}

int SqlRecord::count() const
{
  return sqlRecord.count();
}

void SqlRecord::setEmptyStringsToNull()
{
  for(int i = 0; i < count(); i++)
  {
    if(sqlRecord.field(i).type() == QVariant::String && sqlRecord.value(i).toString().isEmpty())
      sqlRecord.setNull(i);
  }
}

void SqlRecord::appendField(const QString& fieldName, QVariant::Type type)
{
  sqlRecord.append(QSqlField(fieldName, type));
}

void SqlRecord::insertField(int pos, const QString& fieldName, QVariant::Type type)
{
  sqlRecord.insert(pos, QSqlField(fieldName, type));
}

SqlRecord& SqlRecord::appendFieldAndValue(const QString& fieldName, QVariant value)
{
  if(!contains(fieldName))
    appendField(fieldName, value.type());
  setValue(fieldName, value);
  return *this;
}

SqlRecord& SqlRecord::appendFieldAndNullValue(const QString& fieldName, QVariant::Type type)
{
  if(!contains(fieldName))
    appendField(fieldName, type);
  setNull(fieldName);
  return *this;
}

SqlRecord& SqlRecord::insertFieldAndValue(int pos, const QString& fieldName, QVariant value)
{
  if(!contains(fieldName))
    insertField(pos, fieldName, value.type());
  setValue(fieldName, value);
  return *this;
}

SqlRecord& SqlRecord::insertFieldAndNullValue(int pos, const QString& fieldName, QVariant::Type type)
{
  if(!contains(fieldName))
    insertField(pos, fieldName, type);
  setNull(fieldName);
  return *this;
}

SqlRecord& SqlRecord::appendFieldAndValueIf(const QString& fieldName, QVariant value)
{
  if(!value.isNull() && value.isValid())
    appendFieldAndValue(fieldName, value);
  return *this;
}

SqlRecord& SqlRecord::insertFieldAndValueIf(int pos, const QString& fieldName, QVariant value)
{
  if(!value.isNull() && value.isValid())
    insertFieldAndValue(pos, fieldName, value);
  return *this;
}

void SqlRecord::remove(int pos)
{
  sqlRecord.remove(pos);
}

void SqlRecord::remove(const QString& name)
{
  sqlRecord.remove(indexOf(name));
}

void SqlRecord::remove(const QStringList& names)
{
  for(const QString& name : names)
    remove(name);
}

const QStringList SqlRecord::fieldNames() const
{
  QStringList fieldList;

  for(int i = 0; i < count(); i++)
    fieldList.append(fieldName(i));
  return fieldList;
}

const QVariantList SqlRecord::values() const
{
  QVariantList valueList;

  for(int i = 0; i < count(); i++)
    valueList.append(value(i));
  return valueList;
}

void SqlRecord::setValue(int i, const QVariant& val)
{
  if(sqlRecord.fieldName(i).isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");
  sqlRecord.setValue(i, val);
}

void SqlRecord::setValue(const QString& name, const QVariant& val)
{
  if(sqlRecord.indexOf(name) == -1)
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  sqlRecord.setValue(name, val);
}

void SqlRecord::setNull(int i)
{
  if(sqlRecord.fieldName(i).isEmpty())
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field index " + QString::number(i) + " does not exist in query \"" + queryString + "\"");
  sqlRecord.setNull(i);
}

void SqlRecord::setNull(const QString& name)
{
  if(sqlRecord.indexOf(name) == -1)
    throw SqlException(this, QLatin1String(Q_FUNC_INFO) %
                       " Field name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  sqlRecord.setNull(name);
}

void SqlRecord::clear()
{
  sqlRecord.clear();
}

void SqlRecord::clearValues()
{
  sqlRecord.clearValues();
}

QDebug operator<<(QDebug out, const atools::sql::SqlRecord& record)
{
  QDebugStateSaver saver(out);

  out << "SqlRecord[";
  for(int i = 0; i < record.count(); ++i)
    out << record.sqlRecord.fieldName(i) << record.sqlRecord.value(i) << Qt::endl;
  out << "]";

  return out;
}

} // namespace sql
} // namespace atools
