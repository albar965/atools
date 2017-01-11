/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_SQL_SQLRECORD_H
#define ATOOLS_SQL_SQLRECORD_H

#include <QSqlRecord>
#include <QVariant>
#include <QVector>

namespace atools {
namespace sql {

class SqlQuery;

/*
 * Wrapper around SqlRecord that adds additional error checking and exceptions to
 * avoid manual error checking. In case of error or invalid connections SqlException
 * is thrown.
 * Exception is also thrown if a value() receive an invalid
 * name or index.
 */
class SqlRecord
{
public:
  SqlRecord();
  SqlRecord(const QSqlRecord& other, const QString& query = QString());
  SqlRecord(const SqlRecord& other);
  SqlRecord& operator=(const SqlRecord& other);

  ~SqlRecord();

  bool operator==(const SqlRecord& other) const;

  inline bool operator!=(const SqlRecord& other) const
  {
    return !operator==(other);
  }

  QVariant value(int i) const;
  QVariant value(const QString& name) const;

  QString valueStr(int i) const
  {
    return value(i).toString();
  }

  QString valueStr(const QString& name) const
  {
    return value(name).toString();
  }

  int valueInt(int i) const
  {
    return value(i).toInt();
  }

  int valueInt(const QString& name) const
  {
    return value(name).toInt();
  }

  float valueFloat(int i) const
  {
    return value(i).toFloat();
  }

  float valueFloat(const QString& name) const
  {
    return value(name).toFloat();
  }

  int valueBool(int i) const
  {
    return value(i).toBool();
  }

  int valueBool(const QString& name) const
  {
    return value(name).toBool();
  }

  bool isNull(int i) const;
  bool isNull(const QString& name) const;

  int indexOf(const QString& name) const;
  QString fieldName(int i) const;

  bool isGenerated(int i) const;
  bool isGenerated(const QString& name) const;

  bool isEmpty() const;
  bool contains(const QString& name) const;
  int count() const;

  const QSqlRecord& getSqlRecord() const
  {
    return sqlRecord;
  }

  QSqlRecord& getSqlRecord()
  {
    return sqlRecord;
  }

  void appendField(const QString& fieldName, QVariant::Type type);

  void setValue(int i, const QVariant& val);
  void setValue(const QString& name, const QVariant& val);
  void setNull(int i);
  void setNull(const QString& name);

  void clear();
  void clearValues();

  QVariant::Type fieldType(int i) const;
  QVariant::Type fieldType(const QString& name) const;

private:
  QSqlRecord sqlRecord;
  QString queryString;

};

class SqlRecordVector :
  public QVector<SqlRecord>
{

};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLRECORD_H
