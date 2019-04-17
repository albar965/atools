/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "io/inireader.h"

namespace atools {
namespace io {

IniReader::IniReader(const QString& textCodec)
  : AbstractIniReader(textCodec)
{

}

IniReader::~IniReader()
{

}

const IniKeyValues& IniReader::getKeyValuePairs(const QString& section)
{
  static IniKeyValues EMPTY;
  if(keys.contains(section))
    return keys[section];
  else
    return EMPTY;
}

QVariant IniReader::getValue(const QString& section, const QString& key)
{
  return getKeyValuePairs(section).value(key);
}

QString IniReader::getValueString(const QString& section, const QString& key)
{
  return getKeyValuePairs(section).value(key).toString();
}

void IniReader::onKeyValue(const QString& section, const QString& key, const QString& value)
{
  if(!keys.contains(section))
    keys.insert(section, QHash<QString, QVariant> ());

  keys[section].insert(key, value);
}

} // namespace io
} // namespace atools
