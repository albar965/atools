/*
 * SceneryCfg.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

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
  virtual void onKeyValue(const QString& section,
                          const QString& sectionSuffix,
                          const QString& key,
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
