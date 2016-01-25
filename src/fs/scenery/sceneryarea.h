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
