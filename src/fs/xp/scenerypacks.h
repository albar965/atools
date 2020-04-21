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

#ifndef ATOOLS_XP_XPSCENERYPACKS_H
#define ATOOLS_XP_XPSCENERYPACKS_H

#include <QApplication>
#include <QVector>

namespace atools {
namespace fs {
namespace xp {

struct SceneryPack
{
  QString filepath /* File path to apt.dat if exists or base directory if not valid */,
          errorText /* Error message to display if base path is not valid */;

  bool disabled /* Disable by SCENERY_PACK_DISABLED */,
       valid /* File exists */;

  int errorLine /* Line number in file */;
};

/*
 * Reads X-Plane scenery_packs.ini and returns a list with missing paths for error reports and valid
 * entries (disabled or not).
 *
 * X-Plane 11/Custom Scenery/scenery_packs.ini
 *
 * I
 * 1000 Version
 * SCENERY
 *
 * SCENERY_PACK Custom Scenery/X-Plane Landmarks - Chicago/
 * SCENERY_PACK Custom Scenery/2NC0_Mountain_Air_by_hapet/
 * SCENERY_PACK Custom Scenery/A_ENSB_ESCI/
 * SCENERY_PACK Custom Scenery/Aerodrome NTMU Ua_Huka XPFR/
 * SCENERY_PACK Custom Scenery/BIGR_Scenery_Pack/
 *
 * Listing the pack as SCENERY_PACK_DISABLED disables loading entirely.
 * Global Airports are excluded and read separately
 */
class SceneryPacks
{
  Q_DECLARE_TR_FUNCTIONS(XpSceneryPacks)

public:
  SceneryPacks();
  ~SceneryPacks();

  /* Read file and fill entries list. */
  void read(const QString& basePath);

  static bool exists(const QString& basePath, QString& error, QString& filepath);

  /* Get list of entries from file after calling read */
  const QVector<SceneryPack>& getEntries() const;

  /* Get an entry by absolute path. Returns null if it does not exist or path does not exist */
  const SceneryPack *getEntryByPath(const QString& filepath) const;

  int getFileVersion() const
  {
    return fileVersion;
  }

private:
  QVector<SceneryPack> entries;

  /* Absolute path to index in entry list */
  QHash<QString, int> index;
  int fileVersion;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_XPSCENERYPACKS_H
