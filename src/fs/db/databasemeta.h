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

#ifndef ATOOLS_FS_DB_DATABASEMETA_H
#define ATOOLS_FS_DB_DATABASEMETA_H

#include <QDateTime>

namespace atools {
namespace util {
class Version;
}
namespace sql {
class SqlDatabase;
}
namespace fs {
namespace db {

/*
 * Maintains versions and load time for a navdatabases
 */
class DatabaseMeta
{
public:
  DatabaseMeta(atools::sql::SqlDatabase *sqlDb);
  DatabaseMeta(atools::sql::SqlDatabase& sqlDb);

  void logInfo() const;

  /*
   * @return Version that is stored in the database schema
   */
  atools::util::Version getDatabaseVersion() const;

  /*
   * Application version which will be used to create a new database.
   */
  static atools::util::Version getApplicationVersion();

  /*
   * @return timestamp of the last database load
   */
  QDateTime getLastLoadTime() const
  {
    return lastLoadTime;
  }

  /*
   * @return true if the database metadata table was found
   */
  bool isValid() const
  {
    return valid;
  }

  /*
   * @return true if a schema was found (checked by looking for the most important airport table)
   */
  bool hasSchema() const;

  /*
   * @return true if a schema was found and contains data (checked by looking for the most important airport table)
   */
  bool hasData() const;

  bool hasAirspaces() const;

  /* True if any SID or STARs were found (P3D only) */
  bool hasSidStar() const
  {
    return sidStar;
  }

  /* True if table airway has route_type */
  bool hasRouteType() const
  {
    return routeType;
  }

  /*
   * @return true if application major version and database major version are equal
   */
  bool isDatabaseCompatible() const;
  bool isDatabaseCompatible(int major) const;

  /* Update the version information in the database */
  void updateVersion(int majorVer, int minorVer);
  void updateVersion();
  void updateAiracCycle(const QString& cycle, const QString& fromTo);
  void updateAiracCycle();
  void updateDataSource(const QString& src);
  void updateDataSource();

  /* Set database version to application version and timestamp to current time */
  void updateAll();

  void init();

  /* Navdata cycle year and month - Not for FSX/P3D only */
  const QString& getAiracCycle() const
  {
    return airacCycle;
  }

  const QString& getValidThrough() const
  {
    return validThrough;
  }

  void setAiracCycle(const QString& value, const QString& validFromTo = QString())
  {
    airacCycle = value;
    validThrough = validFromTo;
  }

  const QString& getDataSource() const
  {
    return dataSource;
  }

  void setDataSource(const QString& value)
  {
    dataSource = value;
  }

  void updateCompilerVersion();
  void updateCompilerVersion(const QString& versionStr);

  const QString& getCompilerVersion() const
  {
    return compilerVersion;
  }

  void setCompilerVersion(const QString& value)
  {
    compilerVersion = value;
  }

private:
  /* This defines the database schema version of the application and should be updated for every incompatible
   * schema or content change
   */
  static const int DB_VERSION_MAJOR = 14;

  /* Minor database version of the application. Minor version differences are compatible.
   * Since version 10: Fixes in boundary coordinates and indexes added.
   *
   * 1 magnetic variation fix
   * 2 cycle metadata
   * 3 nullable altitude types in boundary
   * 4 nullable path in scenery
   * 5 metadata changes for DFD database
   * 6
   * 7 Fixes for procedure legs
   * 8 airport.is_3d and airport.region added
   * 9 approach_fix_type and runway end altitude added. Fixes for common route in SID/STAR and runway heading.
   * 10 is_3d in airport_medium and airport_large
   * 11 transition_altitude in airport
   * 12 Several changes towards 3.2.
   * 13 Fix for VASI assignment in X-Plane
   * 14 Usage of X-Plane 3D attribute
   * 15 Fix for X-Plane ICAO names
   * 16 "route_type" added to table "airway".
   * 17 Coordinates to procedure legs added.
   *    Fixed issue with duplicate waypoints in airways for DFD, X-Plane and FSX/P3D.
   *    X-Plane navaid range calculation fixed.
   *    Added runway smoothness and airport flatten flags from X-Plane to database schema.
   *    Coordinates for fix and recommended fix in procedure tables added.
   *    Added flag "artificial" for created NDB and VOR waypoints.
   * 18 New FIR and UIR regions deprecating centers
   * 19 Complete MSFS support. New waypoint types and new ramp and gate extra types.
   *    Removed fence and apron light tables. Delete edge and center line light columns from taxipath.
   *    New table translation for MSFS language files.
   * 20 Added faa and local columns to database tables. Remove xpident column.
   * 21 Added arinc_type to tables waypoint and nav_search.
   *    Type and more for ils added. Also added GBAS stations, LPV approaches and more.
   * 22 Rho and theta in approach and transition legs can now be null
   * 23 Table approach_leg.rnp, approach.aircraft_category and path points in DFD compiler.
   * 24 Fixed issues with magnetic variation and inbound course in DFD and X-Plane compiler.
   */
  static const int DB_VERSION_MINOR = 24;

  /* Update the last loaded timestamp in the database and set it to now */
  void updateTimestamp();
  void updateFlags();

  atools::sql::SqlDatabase *db;

  int majorVersion = 0, minorVersion = 0;
  QDateTime lastLoadTime;
  bool valid = false, sidStar = false, routeType = false;
  QString airacCycle, validThrough, dataSource, compilerVersion;
};

} // namespace db
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_DATABASEMETA_H
