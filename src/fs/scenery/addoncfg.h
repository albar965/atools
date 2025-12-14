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

#ifndef ATOOLS_SCENERY_ADDONCFG_H
#define ATOOLS_SCENERY_ADDONCFG_H

#include "fs/scenery/sceneryarea.h"
#include "io/abstractinireader.h"

#include <QList>
#include <QCoreApplication>

namespace atools {
namespace fs {
namespace scenery {

struct AddOnCfgEntry
{
  int packageNum = 0;
  QString path, title;
  bool active = false, required = false, discoveryPath = false;

  bool isValid() const
  {
    return !path.isEmpty();
  }

};

/*
 * Reads flight simulator scenery.cfg entries. Call atools::io::IniReader::read to load a scenery.cfg file.
 * All section and key names are passed in lower case.
 */
class AddOnCfg :
  public atools::io::AbstractIniReader
{
  Q_DECLARE_TR_FUNCTIONS(AddonCfg)

public:
  AddOnCfg(const QString& textCodec);
  virtual ~AddOnCfg() override;

  const QList<atools::fs::scenery::AddOnCfgEntry>& getEntries() const
  {
    return entries;
  }

  QList<atools::fs::scenery::AddOnCfgEntry>& getEntries()
  {
    return entries;
  }

  const QList<atools::fs::scenery::AddOnCfgEntry>& getEntriesDiscovery()
  {
    return entriesDiscovery;
  }

  /* Put a scenery area at the end of the list */
  void setAreaHighPriority(int index, bool value = true);

private:
  virtual void onStartDocument(const QString& filepath) override;
  virtual void onEndDocument(const QString& filepath) override;
  virtual void onStartSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onEndSection(const QString& section, const QString& sectionSuffix) override;
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value) override;

  atools::fs::scenery::AddOnCfgEntry currentEntry;
  QList<atools::fs::scenery::AddOnCfgEntry> entries, entriesDiscovery;

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_SCENERY_ADDONCFG_H
