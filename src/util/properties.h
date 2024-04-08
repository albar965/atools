/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_PROPERTIES_H
#define ATOOLS_PROPERTIES_H

#include <QHash>
#include <QVariant>

class QTextStream;

namespace atools {
namespace util {

/*
 * Simple properties class similar to the Java Properties which also allows to save the content into a similar file format.
 * Values are not escaped.
 *
 * Format is:
 * # Comment
 * Key=Value
 * Key2=Value2
 */
class Properties :
  public QHash<QString, QString>
{
public:
  Properties();
  explicit Properties(const QByteArray& bytes);
  explicit Properties(const QHash<QString, QString>& other);

  bool hasProperty(const QString& name) const
  {
    return contains(name);
  }

  void write(QTextStream& stream) const;
  void read(QTextStream& stream);

  QByteArray asByteArray() const;
  void fromByteArray(const QByteArray& bytes);

  QString writeString() const;
  void readString(QString string);

  QString getPropertyStr(const QString& name, const QString& defaultValue = QString()) const
  {
    return value(name, defaultValue);
  }

  QStringList getPropertyStrList(const QString& name, QChar separator = ',',
                                 const QStringList& defaultValue = QStringList()) const
  {
    return contains(name) ? value(name).split(separator) : defaultValue;
  }

  int getPropertyInt(const QString& name, int defaultValue = 0) const
  {
    return contains(name) ? value(name).toInt() : defaultValue;
  }

  bool getPropertyBool(const QString& name, bool defaultValue = false) const
  {
    return contains(name) ? value(name).toInt() > 0 : defaultValue;
  }

  float getPropertyFloat(const QString& name, float defaultValue = 0.f) const
  {
    return contains(name) ? value(name).toFloat() : defaultValue;
  }

  double getPropertyDouble(const QString& name, double defaultValue = 0.) const
  {
    return contains(name) ? value(name).toDouble() : defaultValue;
  }

  void setPropertyStr(const QString& name, const QString& value)
  {
    insert(name, value);
  }

  void setPropertyStrList(const QString& name, const QStringList& value, QChar separator = ',')
  {
    insert(name, value.join(separator));
  }

  void setPropertyInt(const QString& name, int value)
  {
    insert(name, QString::number(value));
  }

  void setPropertyBool(const QString& name, bool value)
  {
    insert(name, QString::number(value));
  }

  void setPropertyFloat(const QString& name, float value)
  {
    insert(name, QString::number(value, 'f'));
  }

  void setPropertyDouble(const QString& name, double value)
  {
    insert(name, QString::number(value, 'f'));
  }

  static void registerMetaTypes();

};

} // namespace util
} // namespace atools

Q_DECLARE_METATYPE(atools::util::Properties);
Q_DECLARE_TYPEINFO(atools::util::Properties, Q_COMPLEX_TYPE);

#endif // ATOOLS_PROPERTIES_H
