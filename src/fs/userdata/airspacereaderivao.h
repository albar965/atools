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

#ifndef ATOOLS_AIRSPACEREADER_IVAO_JSON_H
#define ATOOLS_AIRSPACEREADER_IVAO_JSON_H

#include "fs/userdata/airspacereaderbase.h"

class QString;

namespace atools {
namespace geo {
class Pos;
}
namespace sql {
class SqlDatabase;
}
namespace fs {
namespace userdata {

/*
 * Reads airspace information from the IVAO JSON format into the table "boundary".
 */
class AirspaceReaderIvao
  : public AirspaceReaderBase
{
public:
  AirspaceReaderIvao(atools::sql::SqlDatabase *sqlDb);
  virtual ~AirspaceReaderIvao() override;

  /* Read a whole file and write airspaces into table. Does not commit. */
  bool readFile(const QString& filenameParam) override;

private:
  /* Convert field "position" to a database type as defined in
   * littlenavmap/src/common/maptypes.cpp in map airspaceTypeFromDatabaseMap */
  QString positionToDbType(const QString& position);

};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRSPACEREADER_IVAO_JSON_H
