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

#include "sql/sqlexport.h"

namespace atools {

namespace sql {

QString SqlExport::getResultSetHeader(const SqlRecord& record) const
{
  QStringList retval;
  for(int i = 0; i < record.count(); i++)
    retval.append(record.fieldName(i));
  return getResultSetHeader(retval);
}

QString SqlExport::getResultSetHeader(const QStringList& strings) const
{
  QString retval;
  for(int i = 0; i < strings.count(); i++)
  {
    QString type = strings.at(i);
    if(i > 0)
    {
      retval += separator;
      retval += buildString(type);
    }
    else
      retval += type;
  }
  retval += printEndl();
  return retval;
}

QString SqlExport::getResultSetRow(const SqlRecord& record) const
{
  QVariantList values;
  for(int i = 0; i < record.count(); i++)
    values.append(record.value(i));
  return getResultSetRow(values);
}

QString SqlExport::getResultSetRow(const QVariantList& values) const
{
  QString retval;

  for(int i = 0; i < values.size(); i++)
  {
    if(i > 0)
      retval += separator;

    retval += printValue(values.at(i));
  }
  retval += printEndl();
  return retval;
}

QString SqlExport::buildString(QString value) const
{
  QString retval = value;

  // Surround the string with " if any special characters or separator are found
  if(value.contains(separator) || value.contains(QChar::LineFeed) || value.contains(QChar::CarriageReturn))
  {
    if(value.contains(escapeString))
      // Escape any inside the string
      retval.replace(QString(escapeString), QString(escapeString) + QString(escapeString));

    retval.prepend(escapeString).append(escapeString);
  }
  return retval;
}

QString SqlExport::printValue(QVariant value) const
{
  QString retval;

  if(!value.isValid() && !value.isNull())
    retval += "[INVALID_VALUE]";
  else if(value.isNull())
    retval += nullValue;
  else
  {
    if(value.canConvert(QVariant::String))
      retval += buildString(value.toString());
    else
    {
      retval += "[CANNOT_CONVERT_VALUE:";
      retval += value.typeName();
      retval += "]";
    }
  }
  return retval;
}

QString SqlExport::printEndl() const
{
  if(endline)
    return "\n";
  else
    return QString();
}

} // namespace sql

} // namespace atools
