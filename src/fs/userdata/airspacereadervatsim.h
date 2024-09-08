/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_AIRSPACEREADER_VATSIM_H
#define ATOOLS_AIRSPACEREADER_VATSIM_H

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
 * Reads airspace information from GeoJSON format into the table "boundary".
 * Note that this is limited to a certain format and does not consider all GeoJSON pecularities.
 *
 * Required geometry type is "MultiPolygon" and all rings except the first one are ignored. No holes.
 * Reads especially VATSIM traconboundaries.json and firboundaries.json .
 *
 * https://datatracker.ietf.org/doc/html/rfc7946
 */
class AirspaceReaderVatsim
  : public AirspaceReaderBase
{
  Q_DECLARE_TR_FUNCTIONS(AirspaceReaderIvao)

public:
  AirspaceReaderVatsim(atools::sql::SqlDatabase *sqlDb);
  virtual ~AirspaceReaderVatsim() override;

  /* Read a whole file and write airspaces into table. Does not commit. */
  bool readFile(const QString& filenameParam) override;

};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRSPACEREADER_VATSIM_H
