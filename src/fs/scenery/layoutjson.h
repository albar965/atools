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

#ifndef ATOOLS_LAYOUTJSON_H
#define ATOOLS_LAYOUTJSON_H

#include <QStringList>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads MSFS layout file and extracts the locations for BGL and material "Library.xml" files.
 * Paths are kept relative as read from file.
 */
class LayoutJson
{
public:
  void read(const QString& filename);

  void clear()
  {
    bglPaths.clear();
    materialPaths.clear();
  }

  /* Relative paths for all BGL files */
  const QStringList& getBglPaths() const
  {
    return bglPaths;
  }

  /* Relative paths for all Library.xml files */
  const QStringList& getMaterialPaths() const
  {
    return materialPaths;
  }

private:
  QStringList bglPaths, materialPaths;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_LAYOUTJSON_H
