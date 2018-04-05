/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_DB_RUNWAYINDEX_H
#define ATOOLS_FS_DB_RUNWAYINDEX_H

#include <QHash>

namespace atools {
namespace fs {
namespace db {

/*
 * Index that maps airport idents and runway names to runway end IDs. This used for each BGL file and does not cross
 * the file boundary.
 */
class RunwayIndex
{
public:
  RunwayIndex()
  {
  }

  virtual ~RunwayIndex();

  /*
   * Adds an entry to the index
   * @param airportIdent airport ICAO ident
   * @param runwayName full runway name like "12L" or "01"
   * @param runwayEndId database runway end ID
   */
  void add(const QString& airportIdent, const QString& runwayName, int runwayEndId);

  /*
   * Get the runway end database ID from the index.
   * @param airportIdent airport ICAO ident
   * @param runwayName full runway name like "12L" or "01"
   * @param sourceObject parent of source object - only used for error reporting
   * @return database runway end ID
   */
  int getRunwayEndId(const QString& airportIdent, const QString& runwayName, const QString& sourceObject);

  void clear()
  {
    runwayIndexMap.clear();
  }

private:
  /* key of airport ident and runway name. Use QPair since it has a hash function */
  typedef QPair<QString, QString> RunwayIndexKeyType;

  typedef QHash<atools::fs::db::RunwayIndex::RunwayIndexKeyType, int> RunwayIndexType;
  typedef atools::fs::db::RunwayIndex::RunwayIndexType::const_iterator RunwayIndexTypeConstIter;

  atools::fs::db::RunwayIndex::RunwayIndexType runwayIndexMap;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_RUNWAYINDEX_H
