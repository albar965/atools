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

#ifndef ATOOLS_SCENERY_SCENERYAREA_H
#define ATOOLS_SCENERY_SCENERYAREA_H

#include <QString>

namespace atools {
namespace fs {
namespace scenery {

/* Collect scenery area entries consisting of more than one BGL files from scenery.cfg
 * Also used to collect entries from add-on.xml files. */
class SceneryArea
{
public:
  SceneryArea();

  /* Constructed from FSX and P3D add-on packages */
  SceneryArea(int areaNum, int layerNum, const QString& sceneryTitle, const QString& sceneryLocalPath);
  SceneryArea(int num, const QString& sceneryTitle, const QString& sceneryLocalPath);

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

  /* Create a tile based on number if empty */
  void fixTitle();

  /* true if add-on should be loaded on top of all others */
  bool isHighPriority() const
  {
    return highPriority;
  }

  void setHighPriority(bool value = true)
  {
    highPriority = value;
  }

  void setAreaNumber(int value)
  {
    areaNumber = value;
  }

  void setTextureId(int value)
  {
    textureId = value;
  }

  void setLayer(int value)
  {
    layer = value;
  }

  void setActive(bool value = true)
  {
    active = value;
  }

  void setRequired(bool value = true)
  {
    required = value;
  }

  void setTitle(const QString& value)
  {
    title = value;
  }

  void setRemotePath(const QString& value)
  {
    remotePath = value;
  }

  void setLocalPath(const QString& value)
  {
    localPath = value;
  }

  void setExclude(const QString& value)
  {
    exclude = value;
  }

  /* MSFS only. Indicates special handling for third-party navdata update scenery areas. */
  bool isMsfsNavigraphNavdata() const
  {
    return msfsNavigraphNavdata;
  }

  void setMsfsNavigraphNavdata(bool value = true)
  {
    msfsNavigraphNavdata = value;
  }

  /* MSFS only. Indicates special handling for "fs-base-nav" navdata update scenery area
   * which add procedures and COM. */
  bool isNavdata() const
  {
    return navdata;
  }

  void setNavdata(bool value = true)
  {
    navdata = value;
  }

  /* MSFS only. Indicates that add-on is stored in the path "Community". */
  bool isCommunity() const
  {
    return community;
  }

  void setCommunity(bool value = true)
  {
    community = value;
  }

  /* MSFS only. Indicates that add-on is stored in the path "Official" and "Steam" or "OneStore" .*/
  bool isAddOn() const
  {
    return addOn;
  }

  void setAddOn(bool value = true)
  {
    addOn = value;
  }

  const QString& getMinGameVersion() const
  {
    return minGameVersion;
  }

  void setMinGameVersion(const QString& value)
  {
    minGameVersion = value;
  }

  const QString& getPackageVersion() const
  {
    return packageVersion;
  }

  void setPackageVersion(const QString& value)
  {
    packageVersion = value;
  }

  /* Included from GUI by path */
  bool isIncluded() const
  {
    return included;
  }

  void setIncluded(bool value)
  {
    included = value;
  }

  /* MSFS 2024: Scenery area where airports and procedures are loaded from SimConnect facility interface */
  bool isSimconnect() const
  {
    return simconnect;
  }

  void setSimconnect(bool value)
  {
    simconnect = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::scenery::SceneryArea& area);

  int areaNumber = 0, textureId = 0, layer = 0;
  bool active = false, required = false, highPriority = false, addOn = false, community = false, included = false,
       msfsNavigraphNavdata = false,
       navdata = false, /* Only MSFS 2020 */
       simconnect = false; /* Only MSFS 2024 */

  QString title, remotePath, localPath, exclude,
          minGameVersion, packageVersion; // Only MSFS
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_SCENERY_SCENERYAREA_H
