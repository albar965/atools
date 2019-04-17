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

#ifndef ATOOLS_SCENERY_SCENERYCFG_H
#define ATOOLS_SCENERY_SCENERYCFG_H

#include "fs/scenery/sceneryarea.h"
#include "io/abstractinireader.h"

#include <QList>
#include <QCoreApplication>

namespace atools {
namespace fs {
namespace scenery {

/*
 * Reads flight simulator scenery.cfg entries. Call atools::io::IniReader::read to load a scenery.cfg file.
 * All section and key names are passed in lower case.
 */
class SceneryCfg :
  public atools::io::AbstractIniReader
{
  Q_DECLARE_TR_FUNCTIONS(SceneryCfg)

public:
  SceneryCfg(const QString& textCodec);
  virtual ~SceneryCfg();

  const QList<atools::fs::scenery::SceneryArea>& getAreas() const
  {
    return areaEntries;
  }

  QList<atools::fs::scenery::SceneryArea>& getAreas()
  {
    return areaEntries;
  }

  void appendArea(const atools::fs::scenery::SceneryArea& area);

  /* Sort areas by layer */
  void sortAreas();

  /* Put a scenery area at the end of the list */
  void setAreaHighPriority(int index, bool value = true);

private:
  virtual void onStartDocument(const QString& filepath) override;
  virtual void onEndDocument(const QString& filepath) override;
  virtual void onStartSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onEndSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value) override;

  atools::fs::scenery::SceneryArea currentArea;
  QString title, description;
  bool cleanOnExit;

  QList<atools::fs::scenery::SceneryArea> areaEntries;

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_SCENERY_SCENERYCFG_H
