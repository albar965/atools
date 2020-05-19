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

#ifndef ATOOLS_FS_LOGDATAMANAGER_H
#define ATOOLS_FS_LOGDATAMANAGER_H

#include "fs/userdata/datamanagerbase.h"

namespace atools {

namespace sql {
class SqlDatabase;
}

namespace fs {
namespace userdata {

/*
 * Contains special functionality around the logbook database.
 */
class LogdataManager :
  public DataManagerBase
{
public:
  LogdataManager(atools::sql::SqlDatabase *sqlDb);
  virtual ~LogdataManager() override;

  /* Import from a custom CSV format which covers all fields in the logbook table. */
  int importCsv(const QString& filepath);

  /* Import and export from a custom CSV format which covers all fields in the logbook table. */
  int exportCsv(const QString& filepath, bool exportPlan, bool exportPerf, bool exportGpx, bool header);

  /* Import X-Plane logbook. Needs a function fetchAirport
   * that resolves airport ident to name and position. */
  int importXplane(const QString& filepath,
                   const std::function<void(atools::geo::Pos& pos, QString& name,
                                            const QString& ident)>& fetchAirport);

  /* Update schema to latest. Checks for new columns and tables. */
  void updateSchema();

  /* Get various statistical information for departure times */
  void getFlightStatsTime(QDateTime& earliest, QDateTime& latest, QDateTime& earliestSim, QDateTime& latestSim);

  /* Flight plant distances in NM for logbook entries */
  void getFlightStatsDistance(float& distTotal, float& distMax, float& distAverage);

  /* Trip Time in hours */
  void getFlightStatsTripTime(float& timeMaximum, float& timeAverage, float& timeMaximumSim, float& timeAverageSim);

  /* Various numbers */
  void getFlightStatsAirports(int& numDepartAirports, int& numDestAirports);
  void getFlightStatsAircraft(int& numTypes, int& numRegistrations, int& numNames, int& numSimulators);

  /* Simulator to number of logbook entries */
  void getFlightStatsSimulator(QVector<std::pair<int, QString> >& numSimulators);

  /* Fills null fields with empty strings to avoid issue when searching */
  static void fixEmptyFields(atools::sql::SqlRecord& rec);
  static void fixEmptyFields(atools::sql::SqlQuery& query);

private:
  static void fixEmptyStrField(atools::sql::SqlRecord& rec, const QString& name);
  static void fixEmptyStrField(atools::sql::SqlQuery& query, const QString& name);

  /* Convert Gzipped BLOB to text (file) */
  static QString blobConversionFunction(const QVariant& value);

  /* Generate empty column if disabled in export options */
  static QString blobConversionFunctionEmpty(const QVariant&);

};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_LOGDATAMANAGER_H
