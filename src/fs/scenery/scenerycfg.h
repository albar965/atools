/*
 * SceneryCfg.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef SCENERY_SCENERYCFG_H_
#define SCENERY_SCENERYCFG_H_

#include "sceneryarea.h"
#include "inireader.h"

#include <QString>
#include <QList>

namespace atools {
namespace fs {
namespace scenery {

typedef QList<SceneryArea> AreaVectorType;
typedef QList<SceneryArea>::const_iterator AreaVectorIterType;

class SceneryCfg :
  public IniReader
{
public:
  SceneryCfg()
    : IniReader(), cleanOnExit(false)
  {
  }

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
  SceneryArea currentArea;
  QString title, description;
  bool cleanOnExit;

  AreaVectorType areaEntries;
  bool toBool(const QString& str);
  int toInt(const QString& str);

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* SCENERY_SCENERYCFG_H_ */
