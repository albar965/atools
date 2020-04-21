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

#ifndef ATOOLS_INIREADER_H
#define ATOOLS_INIREADER_H

#include "io/abstractinireader.h"

#include <QHash>
#include <QVariant>

namespace atools {
namespace io {

typedef QHash<QString, QVariant> IniKeyValues;

/*
 *  Reads INI style classes without the issues of QSettings (forced write back).
 */
class IniReader :
  public atools::io::AbstractIniReader
{
public:
  IniReader(const QString& textCodec = QString());
  virtual ~IniReader() override;

  /* Get all key value pairs for a [section] */
  const IniKeyValues& getKeyValuePairs(const QString& section);

  /* Get value for a [section] and key=value */
  QVariant getValue(const QString& section, const QString& key);
  QString getValueString(const QString& section, const QString& key);

protected:
  virtual void onKeyValue(const QString& section, const QString& key, const QString& value) override;

  /* Map of sections to keys to values */
  QHash<QString, QHash<QString, QVariant> > keys;
};

} // namespace io
} // namespace atools

#endif // ATOOLS_INIREADER_H
