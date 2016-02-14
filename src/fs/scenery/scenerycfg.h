/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef SCENERY_SCENERYCFG_H_
#define SCENERY_SCENERYCFG_H_

#include "fs/scenery/sceneryarea.h"
#include "fs/scenery/inireader.h"

#include <QList>

namespace atools {
namespace fs {
namespace scenery {

typedef QList<atools::fs::scenery::SceneryArea> AreaVectorType;
typedef QList<atools::fs::scenery::SceneryArea>::const_iterator AreaVectorIterType;

class SceneryCfg :
  public atools::fs::scenery::IniReader
{
public:
  SceneryCfg();

  virtual ~SceneryCfg();

  virtual void onStartDocument(const QString& filename) override;
  virtual void onEndDocument(const QString& filename) override;
  virtual void onStartSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onEndSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value) override;

  const AreaVectorType& getAreas() const
  {
    return areaEntries;
  }

private:
  atools::fs::scenery::SceneryArea currentArea;
  QString title, description;
  bool cleanOnExit;

  atools::fs::scenery::AreaVectorType areaEntries;
  bool toBool(const QString& str);
  int toInt(const QString& str);

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* SCENERY_SCENERYCFG_H_ */
