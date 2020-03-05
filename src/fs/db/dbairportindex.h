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

#ifndef ATOOLS_FS_DB_AIRPORTINDEX_H
#define ATOOLS_FS_DB_AIRPORTINDEX_H

#include <QHash>

namespace atools {
namespace fs {
namespace db {

/*
 * Index that maps airport idents airport IDs. This used for each BGL file and does not cross
 * the file boundary.
 */
class DbAirportIndex
{
public:
  DbAirportIndex()
  {
  }

  virtual ~DbAirportIndex();

  /*
   * Adds an entry to the index
   * @param airportIdent airport ICAO ident
   * @param airportId database runway end ID
   */
  void add(const QString& airportIdent, int airportId);

  /*
   * Get the runway end database ID from the index.
   * @param airportIdent airport ICAO ident
   * @param sourceObject parent of source object - only used for error reporting
   * @return database airport ID
   */
  int getAirportId(const QString& airportIdent, const QString& sourceObject);

  void clear()
  {
    airportIndexMap.clear();
  }

private:
  typedef QHash<QString, int> AirportIndexType;
  typedef atools::fs::db::DbAirportIndex::AirportIndexType::const_iterator AirportIndexTypeConstIter;

  atools::fs::db::DbAirportIndex::AirportIndexType airportIndexMap;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AIRPORTINDEX_H
