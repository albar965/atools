/*
 * SceneryEntry.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef SCENERY_SCENERYAREA_H_
#define SCENERY_SCENERYAREA_H_

#include <QString>

namespace atools {
namespace fs {
namespace scenery {

class SceneryArea
{
public:
  SceneryArea()
    : areaNumber(0), textureId(0), layer(0), active(false), required(false)
  {
  }

  virtual ~SceneryArea();

  bool isActive() const
  {
    return active;
  }

  const QString& getExclude() const
  {
    return exclude;
  }

  int getLayer() const
  {
    return layer;
  }

  const QString getLocalPath() const;

  const QString& getRemotePath() const
  {
    return remotePath;
  }

  bool isRequired() const
  {
    return required;
  }

  int getTextureId() const
  {
    return textureId;
  }

  int getAreaNumber() const
  {
    return areaNumber;
  }

  const QString& getTitle() const
  {
    return title;
  }

private:
  friend class SceneryCfg;
  friend QDebug operator<<(QDebug out, const atools::fs::scenery::SceneryArea& area);

  int areaNumber;
  QString title;
  int textureId;
  QString remotePath;
  QString localPath;
  int layer;
  bool active;
  bool required;
  QString exclude;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* SCENERY_SCENERYAREA_H_ */
