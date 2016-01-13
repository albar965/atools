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

#ifndef ATOOLS_FS_AP_AIRPORTLOADER_H
#define ATOOLS_FS_AP_AIRPORTLOADER_H

#include "sql/sqlquery.h"

#include <QXmlStreamReader>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace ap {

/*
 * Loads the runways.xml file into a SQL database.
 * Only a limited set of airport information is loaded.
 */
class AirportLoader
{
public:
  /*
   * @param sqlDb Database to use
   */
  AirportLoader(atools::sql::SqlDatabase *sqlDb);
  ~AirportLoader();

  /*
   * Creates all tables and indexes and loads the airport information into the
   * database.
   * @param filename the runways.xml file
   * Throws Exception in case of error.
   */
  void loadAirports(const QString& filename);

  /*
   * @return number of airports loaded
   */
  int getNumLoaded() const
  {
    return numLoaded;
  }

  /* Drops all tables and views */
  void dropDatabase();

private:
  atools::sql::SqlDatabase *db;
  atools::sql::SqlQuery *query;
  int numLoaded = 0;
  QXmlStreamReader reader;

  void readData();
  void readIcao();
  void readRunway();

};

} // namespace fs
} // namespace ap
} // namespace atools

#endif // ATOOLS_FS_AP_AIRPORTLOADER_H
