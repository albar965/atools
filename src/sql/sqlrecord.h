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

#ifndef ATOOLS_SQL_SQLRECORD_H
#define ATOOLS_SQL_SQLRECORD_H

#include <QSqlRecord>
#include <QVariant>
#include <QVector>

class QSqlRecord;

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
  SqlRecord()
  {

  }

  SqlRecord(const QSqlRecord& other, const QString& query = QString())
  {
    sqlRecord = other;
    queryString = query;
  }

  SqlRecord(const SqlRecord& other)
  {
    sqlRecord = other.sqlRecord;
    queryString = other.queryString;

  }

  SqlRecord& operator=(const SqlRecord& other)
  {
    sqlRecord = other.sqlRecord;
    queryString = other.queryString;
    return *this;
  }

  bool operator==(const SqlRecord& other) const
  {
    return sqlRecord == other.sqlRecord;
  }

  inline bool operator!=(const SqlRecord& other) const
  {
    return !operator==(other);
  }

  /* General getters. Throw exception if value does not exist as field. */
  QVariant value(int i) const;
  QVariant value(const QString& name) const;

  /* Typed getters. Throw exception if value does not exist as field. */
  QString valueStr(int i) const
  {
    return value(i).toString();
  }

  QString valueStr(const QString& name) const
  {
    return value(name).toString();
  }

  QChar valueChar(int i) const
  {
    return value(i).toChar();
  }

  QChar valueChar(const QString& name) const
  {
    return value(name).toChar();
  }

  QDateTime valueDateTime(int i) const;

  QDateTime valueDateTime(const QString& name) const;

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

  double valueDouble(int i) const
  {
    return value(i).toDouble();
  }

  double valueDouble(const QString& name) const
  {
    return value(name).toDouble();
  }

  int valueBool(int i) const
  {
    return value(i).toBool();
  }

  int valueBool(const QString& name) const
  {
    return value(name).toBool();
  }

  QByteArray valueBytes(int i) const
  {
    return value(i).toByteArray();
  }

  QByteArray valueBytes(const QString& name) const
  {
    return value(name).toByteArray();
  }

  /* Getters which return a default value if field does not exist in record instead of throwing an exception. */
  QVariant value(const QString& name, const QVariant& defaulValue) const
  {
    return contains(name) ? value(name) : defaulValue;
  }

  QString valueStr(const QString& name, const QString& defaultValue) const
  {
    return contains(name) ? valueStr(name) : defaultValue;
  }

  int valueInt(const QString& name, int defaultValue) const
  {
    return contains(name) ? valueInt(name) : defaultValue;
  }

  float valueFloat(const QString& name, float defaultValue) const
  {
    return contains(name) ? valueFloat(name) : defaultValue;
  }

  double valueDouble(const QString& name, double defaultValue) const
  {
    return contains(name) ? valueDouble(name) : defaultValue;
  }

  int valueBool(const QString& name, bool defaultValue) const
  {
    return contains(name) ? valueBool(name) : defaultValue;
  }

  QByteArray valueBytes(const QString& name, const QByteArray& defaultValue) const
  {
    return contains(name) ? valueBytes(name) : defaultValue;
  }

  QDateTime valueDateTime(const QString& name, const QDateTime& defaultValue) const;

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

  /* Set all empty string values to null */
  void setEmptyStringsToNull();

  void appendField(const QString& fieldName, QVariant::Type type);
  void insertField(int pos, const QString& fieldName, QVariant::Type type);

  /* Adds field if not exists and value too. Type is derived from value */
  SqlRecord& appendFieldAndValue(const QString& fieldName, QVariant value);
  SqlRecord& appendFieldAndNullValue(const QString& fieldName, QVariant::Type type);
  SqlRecord& insertFieldAndValue(int pos, const QString& fieldName, QVariant value);
  SqlRecord& insertFieldAndNullValue(int pos, const QString& fieldName, QVariant::Type type);

  /* Adds field if not exists and value too if value. Type is derived from value */
  SqlRecord& appendFieldAndValueIf(const QString& fieldName, QVariant value);
  SqlRecord& insertFieldAndValueIf(int pos, const QString& fieldName, QVariant value);

  void setValue(int i, const QVariant& val);
  void setValue(const QString& name, const QVariant& val);
  void setNull(int i);
  void setNull(const QString& name);

  void clear();
  void clearValues();

  QVariant::Type fieldType(int i) const;
  QVariant::Type fieldType(const QString& name) const;

  void remove(int pos);
  void remove(const QString& name);

  void remove(const QStringList& names);

  const QStringList fieldNames() const;
  const QVariantList values() const;

  const QString& getQueryString() const
  {
    return queryString;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::sql::SqlRecord& record);

  QSqlRecord sqlRecord;
  QString queryString;

};

QDebug operator<<(QDebug out, const atools::sql::SqlRecord& record);

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLRECORD_H
