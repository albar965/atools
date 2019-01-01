/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_COMMON_MORAREADER_H
#define ATOOLS_FS_COMMON_MORAREADER_H

#include <QString>
#include <QVector>

namespace atools {
namespace geo {
class Pos;
}
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace common {

/*
 * Provides methods to read, write and access the MORA (minimum off route altitude) data.
 *
 * Grid MORA values clear all terrain and obstructions by 1000 feet in areas where
 * the highest elevations are 5000 feet MSL or lower.
 * MORA values clear all terrain by 2000 feet in areas where the highest elevations are 5001 feet MSL or higher.
 *
 * The field will contain values expressed in hundreds of feet, for example, the value of 6000 feet is expressed as 060 and the value of 7100 feet is expressed as 071. For geographical sections that are not surveyed, the field will contain the alpha characters UNK for Unknown.
 */
class MoraReader
{
public:
  MoraReader(atools::sql::SqlDatabase *sqlDb);
  MoraReader(atools::sql::SqlDatabase& sqlDb);
  virtual ~MoraReader();

  /* Read values from table "mora_grid". returns true if successfull and table exists. */
  bool readFromTable();

  /* Sets database and reads as above */
  bool readFromTable(sql::SqlDatabase& sqlDb);

  /* Writes values to table "mora_grid". Object has to be valid. Copies data to this instance. */
  void writeToTable(const QVector<quint16>& datagrid, int columns, int rows);

  /* True if table is present in schema and has one row */
  bool isDataAvailable();

  /* Frees memory and sets state to invalid */
  void clear();

  /* true if loaded */
  bool isValid() const
  {
    return !datagrid.isEmpty();
  }

  /* Returns minimum off route altitude at position in feet * 100, UNKNOWN, ERROR or OCEAN.
   * Throws exception if object is not valid.
   *
   * Coordinates are top left corner of rectangle.
   * -180 <= x <= 179
   * -89 <= y <= 90
   */
  int getMoraFt(const atools::geo::Pos& pos) const;
  int getMoraFt(int lonx, int laty) const;

  /* Not surveyed */
  const static quint16 UNKNOWN = std::numeric_limits<quint16>::max();

  /* Error reading */
  const static quint16 ERROR = std::numeric_limits<quint16>::max() - 1;

  /* Ocean / not set */
  const static quint16 OCEAN = 0;

private:
  atools::sql::SqlDatabase *db;
  bool dataAvailable = false;
  QVector<quint16> datagrid;
  int lonxColums = 0, latyRows = 0;

  const static quint32 MAGIC_NUMBER_DATA = 0xA5B44CDB;
  const static quint32 DATA_VERSION = 1;

};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_COMMON_MORAREADER_H
