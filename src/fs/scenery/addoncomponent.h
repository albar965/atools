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

#ifndef ATOOLS_FS_SCENERY_ADDONCOMPONENT_H
#define ATOOLS_FS_SCENERY_ADDONCOMPONENT_H

#include <QString>

class QXmlStreamReader;

namespace atools {
namespace fs {
namespace scenery {

/*
 * A component of Prepar3D v4 add-on XML files.
 * Only scenery components are read - all other are ignored.
 */
class AddOnComponent
{
public:
  AddOnComponent();
  AddOnComponent(QXmlStreamReader& xml);

  const QString& getCategory() const
  {
    return category;
  }

  const QString& getPath() const
  {
    return path;
  }

  const QString& getName() const
  {
    return name;
  }

  int getLayer() const
  {
    return layer;
  }

private:
  QString category, path, name;
  int layer = -1;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SCENERY_ADDONCOMPONENT_H
