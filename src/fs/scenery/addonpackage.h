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

#ifndef ATOOLS_FS_SCENERY_ADDONPACKAGE_H
#define ATOOLS_FS_SCENERY_ADDONPACKAGE_H

#include "fs/scenery/addoncomponent.h"

#include <QString>
#include <QVector>
#include <QApplication>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads the Prepar3D v4 add-on XML files.
 * Only scenery components are read - all other are ignored.
 */
class AddOnPackage
{
  Q_DECLARE_TR_FUNCTIONS(AddOnPackage)

public:
  AddOnPackage(const QString& file);
  ~AddOnPackage();

  const QVector<atools::fs::scenery::AddOnComponent>& getComponents() const
  {
    return components;
  }

  /* Directory of this file */
  const QString& getBaseDirectory() const
  {
    return baseDirectory;
  }

  const QString& getName() const
  {
    return name;
  }

  const QString& getDescription() const
  {
    return description;
  }

  const QString& getFilename() const
  {
    return filename;
  }

private:
  QString filename, baseDirectory, name, description;

  QVector<atools::fs::scenery::AddOnComponent> components;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SCENERY_ADDONPACKAGE_H
