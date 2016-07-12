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

#ifndef ATOOLS_SCENERY_SCENERYAREA_H
#define ATOOLS_SCENERY_SCENERYAREA_H

#include <QString>

namespace atools {
namespace fs {
namespace scenery {

class SceneryArea
{
public:
  SceneryArea();

  virtual ~SceneryArea();

  /*
   * @return true to indicate that the scenery should be rendered by default.
   */
  bool isActive() const
  {
    return active;
  }

  /* exclude value */
  const QString& getExclude() const
  {
    return exclude;
  }

  /*
   * @return The layer  number is typically equal to the [area.nnn] number.
   * Higher numbered layers have priority over lower numbered layers.
   */
  int getLayer() const
  {
    return layer;
  }

  /*
   * @return The local path to the scenery files.
   */
  const QString& getLocalPath() const
  {
    return localPath;
  }

  /* Remote path is used for slow media */
  const QString& getRemotePath() const
  {
    return remotePath;
  }

  /*
   * @return true to indicate the folder is required and that the scenery entry
   * cannot be deleted or turned off in the Scenery Library dialog.
   */
  bool isRequired() const
  {
    return required;
  }

  /*
   * @return [area.nnn] number.
   */
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

#endif // ATOOLS_SCENERY_SCENERYAREA_H
