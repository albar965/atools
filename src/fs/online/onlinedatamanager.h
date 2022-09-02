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

#ifndef ATOOLS_ONLINEDATAMANAGER_H
#define ATOOLS_ONLINEDATAMANAGER_H

#include "fs/online/onlinetypes.h"
#include "sql/sqltypes.h"

#include <QString>

class QDateTime;

namespace atools {
namespace geo {
class Pos;
}

namespace sql {
class SqlDatabase;
class SqlQuery;
class SqlRecord;
}

namespace fs {
namespace sc {
class SimConnectAircraft;
}

namespace online {

class StatusTextParser;
class WhazzupTextParser;

/*
 * Facade for online classes that parse whazzup.txt and status.txt files for IVAO, VATSIM or other muultiplayer
 * services.
 *
 * All content from whazzup.txt is written into the given database.
 *
 * Check for schema and create this before reading.
 */
class OnlinedataManager
{

public:
  OnlinedataManager(atools::sql::SqlDatabase *sqlDb, bool verboseErrorReporting);
  ~OnlinedataManager();

  OnlinedataManager(const OnlinedataManager& other) = delete;
  OnlinedataManager& operator=(const OnlinedataManager& other) = delete;

  /* Read status.txt and populate internal list of URLs and message. File content given in string. */
  void readFromStatus(const QString& statusTxt);

  /* Read all from whazzup.txt or VATSIM JSON 3 file with file content in string and writes all into the database.
   * Returns true if the file was read and is more recent than lastUpdate. */
  bool readFromWhazzup(const QString& whazzupTxt, Format format, const QDateTime& lastUpdate);

  /* Read VATSIM transceivers-data.json and stores map in this object. Call before calling "readFromWhazzup" */
  void readFromTransceivers(const QString& transceiverTxt);

  /* Read all servers and voice_servers from whazzup.txt file with file content in string and writes all into the database */
  bool readServersFromWhazzup(const QString& whazzupTxt, Format format, const QDateTime& lastUpdate);

  /* Get a randon URL from the status file which points to the redundant whazzup files */
  QString getWhazzupUrlFromStatus(bool& gzipped, bool& json) const;

  /* Get a randon voice server URL from the status file which points to the redundant whazzup files */
  QString getWhazzupVoiceUrlFromStatus() const;

  /* Get message which has to be shown on application start */
  const QString& getMessageFromStatus();

  /* The last date and time this file has been updated. */
  QDateTime getLastUpdateTimeFromWhazzup() const;

  /* Time in minutes this file will be updated */
  int getReloadMinutesFromWhazzup() const;

  /* True if table clients is present in database */
  bool hasSchema();

  /* True if table userdata is presend in database and is filled*/
  bool hasData();

  /* Create database schema. Drops current schema if tables already exist. */
  void createSchema();

  /* Remove all data from the tables. */
  void clearData();

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

  /* Drops schema tables and indexes */
  void dropSchema();

  /* Create all queries */
  void initQueries();

  /* Delete all queries */
  void deInitQueries();

  /* Reset all URLs and timestamps. Does not reset the tables for semi-permanent ids */
  void reset();

  /* Reset all URLs and timestamps and the tables for semi-permanent ids */
  void resetForNewOptions();

  /* Fill from a record based on table client */
  static void fillFromClient(atools::fs::sc::SimConnectAircraft& ac, const atools::sql::SqlRecord& record,
                             const sc::SimConnectAircraft& simShadowAircraft);

  /* Get aircraft from table client by id. */
  atools::fs::sc::SimConnectAircraft  getClientAircraftById(int clientId);

  /* Get aircraft from table client by id and fill additional data from simAircraft into aircraft class. */
  atools::fs::sc::SimConnectAircraft getClientAircraft(const atools::fs::sc::SimConnectAircraft& simAircraft);

  /* Get all rows for a client_id */
  atools::sql::SqlRecord getClientRecordById(int clientId);

  /* Get all rows for clients that match the callsign. Normally only one. */
  atools::sql::SqlRecordList getClientRecordsByCallsign(const QString& callsign);

  /* Fill the map with callsign as key and position as value. Used for online/simulator deduplication. */
  QVector<atools::fs::online::OnlineAircraft> getClientCallsignAndPosMap();

  /* Number of client aircraft in client table */
  int getNumClients() const;

  /* Set default circle radii for certain ATC types where visual range is unusable */
  void setAtcSize(const QHash<atools::fs::online::fac::FacilityType, int>& value);

  /* Set a callback that tries to fetch geometry from the user airspace database.
   * Default circle will be used if this returns an empty byte array or a null pointer. */
  void setGeometryCallback(atools::fs::online::GeoCallbackType func);

private:
  atools::sql::SqlDatabase *db;

  atools::fs::online::WhazzupTextParser *whazzup = nullptr;
  atools::fs::online::WhazzupTextParser *whazzupServers = nullptr;
  atools::fs::online::StatusTextParser *status = nullptr;

};

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_ONLINEDATAMANAGER_H
