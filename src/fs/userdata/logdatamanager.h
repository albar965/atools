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

#include "sql/datamanagerbase.h"
#include "geo/linestring.h"

#include <QCache>

namespace atools {
namespace geo {
class LineString;
}
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace userdata {

/* Flight plan geometry, waypoint names and track geometry for a logbook entry */
struct LogEntryGeometry
{
  atools::geo::LineString route;
  QVector<atools::geo::LineString> tracks;
  QVector<QVector<quint32> > timestamps;

  /* Flight plan waypoint names. String list has the same size as route */
  QStringList names;
  atools::geo::Rect routeRect, trackRect;
};

/*
 * Contains special functionality around the logbook database.
 */
class LogdataManager :
  public atools::sql::DataManagerBase
{
public:
  LogdataManager(atools::sql::SqlDatabase *sqlDb);
  virtual ~LogdataManager() override;

  /* Import from a custom CSV format which covers all fields in the logbook table. */
  int importCsv(const QString& filepath);

  /* Import and export from a custom CSV format which covers all fields in the logbook table. */
  int exportCsv(const QString& filepath, const QVector<int>& ids, bool exportPlan, bool exportPerf,
                bool exportGpx, bool header, bool append);

  /* Import X-Plane logbook. Needs a function fetchAirport
   * that resolves airport ident to name and position. */
  int importXplane(const QString& filepath,
                   const std::function<void(atools::geo::Pos& pos, QString& name,
                                            const QString& ident)>& fetchAirport);

  /* Update schema to latest. Checks for new columns and tables. */
  void updateSchema();

  /* Get flight plan and track and route points from GPX attachment or database BLOB. Request is cached.
   *  Also includes route waypoint names. */
  const atools::fs::userdata::LogEntryGeometry *getGeometry(int id);

  /* Clear cache used by getRouteGeometry and getTrackGeometry */
  void clearGeometryCache();

  /* true if any of the files/BLOBs is present (length > 0) for the dataset */
  bool hasRouteAttached(int id);
  bool hasPerfAttached(int id);
  bool hasTrackAttached(int id);

  /* Get various statistical information for departure times */
  void getFlightStatsTime(QDateTime& earliest, QDateTime& latest, QDateTime& earliestSim, QDateTime& latestSim);

  /* Flight plant distances in NM for logbook entries */
  void getFlightStatsDistance(float& distTotal, float& distMax, float& distAverage);

  /* Trip Time in hours */
  void getFlightStatsTripTime(float& timeMaximum, float& timeAverage, float& timeTotal, float& timeMaximumSim,
                              float& timeAverageSim, float& timeTotalSim);

  /* Various numbers */
  void getFlightStatsAirports(int& numDepartAirports, int& numDestAirports);
  void getFlightStatsAircraft(int& numTypes, int& numRegistrations, int& numNames, int& numSimulators);

  /* Simulator to number of logbook entries */
  void getFlightStatsSimulator(QVector<std::pair<int, QString> >& numSimulators);

  /* Fills null fields with empty strings to avoid issue when searching */
  static void fixEmptyFields(atools::sql::SqlRecord& rec);
  static void fixEmptyFields(atools::sql::SqlQuery& query);

  static const int MAX_CACHE_ENTRIES = 100;

private:
  static void fixEmptyStrField(atools::sql::SqlRecord& rec, const QString& name);
  static void fixEmptyStrField(atools::sql::SqlQuery& query, const QString& name);
  static void fixEmptyBlobField(atools::sql::SqlRecord& rec, const QString& name);
  static void fixEmptyBlobField(atools::sql::SqlQuery& query, const QString& name);

  /* Convert Gzipped BLOB to text (file) */
  static QString blobConversionFunction(const QVariant& value);

  /* Generate empty column if disabled in export options */
  static QString blobConversionFunctionEmpty(const QVariant&);

  /* Prime cache by loading the GpxCacheEntry */
  void loadGpx(int id);

  /* Cache to avoid reading BLOBs */
  QCache<int, LogEntryGeometry> cache;

};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_LOGDATAMANAGER_H
