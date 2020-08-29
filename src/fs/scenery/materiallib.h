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

#ifndef ATOOLS_MATERIALLIB_H
#define ATOOLS_MATERIALLIB_H

#include <QHash>
#include <QUuid>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads MSFS material library XML and maps GUIDs to material name if not "UNDEFINED"
 */
class MaterialLib
{
public:
  /* Read a material library from a community package by looking into the layout.json file
   *  @param basePath Path containing the layout file. */
  void readCommunity(const QString& basePath);

  /*
   * Read the two official material libraries in asobo-material-lib and fs-base-material-lib. Merge results to map.
   * @param basePath is one below
   * C:\Users\alex\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages\Official\OneStore
   */
  void readOfficial(const QString& basePath);

  /* Read material from given file */
  void read(const QString& filename);

  void clear()
  {
    surfaceMap.clear();
  }

  /* Get a long surface name like "ASPHALT" for the given UUID/GUID */
  QString getSurfaceForUuid(const QUuid& uuid) const
  {
    return surfaceMap.value(uuid);
  }

private:
  QHash<QUuid, QString> surfaceMap;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_MATERIALLIB_H
