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

#ifndef ATOOLS_AIRSPACEREADER_OPENAIR_H
#define ATOOLS_AIRSPACEREADER_OPENAIR_H

#include "geo/linestring.h"

#include "fs/userdata/airspacereaderbase.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}
namespace fs {
namespace userdata {

/*
 * Reads OpenAir files containing airspaces and writes them to the boundary table.
 *
 * http://www.winpilot.com/UsersGuide/UserAirspace.asp
 */
class AirspaceReaderOpenAir :
  public AirspaceReaderBase
{
public:
  AirspaceReaderOpenAir(sql::SqlDatabase *sqlDb);
  virtual ~AirspaceReaderOpenAir() override;

  /* Read a whole file and write airspaces into table */
  virtual void readFile(int fileIdParam, const QString& filenameParam) override;

  /* Read a line from a file and write to file if end of airspace detected */
  virtual void readLine(const QStringList& line, int fileIdParam, const QString& filenameParam,
                        int lineNumberParam) override;

  /* Writes last airspace to table */
  virtual void finish() override;

  /* reset internal values back */
  virtual void reset() override;

private:
  void writeBoundary();
  void bindAltitude(const QStringList& line, bool isMax);
  void bindClass(const QString& cls);
  void bindName(const QString& name);
  void bindCoordinate(const QStringList& line);

  bool writingCoordinates = false;
  atools::geo::LineString curLine;
  atools::geo::Pos center;
  bool clockwise = true;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRSPACEREADER_OPENAIR_H
