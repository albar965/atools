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

#ifndef ATOOLS_LANGUAGEJSON_H
#define ATOOLS_LANGUAGEJSON_H

#include <QHash>
#include <QString>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads MSFS language files like
 * ".../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore/fs-base/en-US.locPak"
 * and creates a map for airport names like "TT:AIRPORTXX.MYNN.name" to the real localized name.
 */
class LanguageJson
{
public:
  LanguageJson(const QString& filename);

  /* Get localized airport name from key found in BGL file like "TT:AIRPORTXX.MYNN.name" */
  QString getName(QString key) const;

private:
  QHash<QString, QString> names;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_LANGUAGEJSON_H
