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

#ifndef ATOOLS_CONTENTXML_H
#define ATOOLS_CONTENTXML_H

#include <QApplication>
#include <QSet>
#include <QVector>

namespace atools {
namespace fs {
namespace scenery {

class SceneryArea;

/*
 * Reads MSFS content.xml file and creates a list of scenery areas.
 * This will include only the official packages.
 */
class ContentXml
{
  Q_DECLARE_TR_FUNCTIONS(ContentXml)

public:
  /* Read the file and add fs-base and fs-base-nav packages */
  void read(const QString& filename);

  const QList<atools::fs::scenery::SceneryArea>& getAreas() const
  {
    return areaEntries;
  }

  QList<atools::fs::scenery::SceneryArea>& getAreas()
  {
    return areaEntries;
  }

  /* true if area is present and has active="false" */
  bool isDisabled(const QString& areaPath) const
  {
    return disabledAreas.contains(areaPath.toLower());
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::scenery::ContentXml& cfg);

  QList<atools::fs::scenery::SceneryArea> areaEntries;
  QSet<QString> disabledAreas;
  int number = 0;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_CONTENTXML_H
