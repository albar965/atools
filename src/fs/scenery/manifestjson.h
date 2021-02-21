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

#ifndef ATOOLS_MANIFESTJSON_H
#define ATOOLS_MANIFESTJSON_H

#include <QString>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads MSFS manifest file and extracts most important fields.
 *
 * Example:
 *  {
 *  "dependencies": [],
 *  "content_type": "SCENERY",
 *  "title": "Navigraph Navdata Cycle 2010-revision.10",
 *  "manufacturer": "",
 *  "creator": "Navigraph",
 *  "package_version": "0.1.0",
 *  "minimum_game_version": "1.8.3",
 *  "release_notes": {
 *   "neutral": {
 *     "LastUpdate": "",
 *     "OlderHistory": ""
 *   }
 *  }
 */
class ManifestJson
{
public:
  void read(const QString& filename);

  /* Might be scenery or core */
  bool isScenery() const;

  const QString& getContentType() const
  {
    return contentType;
  }

  const QString& getTitle() const
  {
    return title;
  }

  const QString& getManufacturer() const
  {
    return manufacturer;
  }

  const QString& getCreator() const
  {
    return creator;
  }

  const QString& getPackageVersion() const
  {
    return packageVersion;
  }

  void clear()
  {
    contentType.clear();
    title.clear();
    manufacturer.clear();
    creator.clear();
    packageVersion.clear();
  }

private:
  QString contentType, title, manufacturer, creator, packageVersion;

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_MANIFESTJSON_H
