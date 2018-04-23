/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include <QString>

class QDateTime;

namespace atools {
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
  OnlinedataManager(atools::sql::SqlDatabase *sqlDb);
  ~OnlinedataManager();

  /* Read status.txt and populate internal list of URLs and message. File content given in string. */
  void readFromStatus(const QString& statusTxt);

  /* Read all from whazzup.txt file with file content in string and writes all into the database.
   * Returns true if the file was read and is more recent than lastUpdate. */
  bool readFromWhazzup(const QString& whazzupTxt, Format format, const QDateTime& lastUpdate);

  /* Read all servers and voice_servers from whazzup.txt file with file content in string and writes all into the database */
  bool readServersFromWhazzup(const QString& whazzupTxt, Format format, const QDateTime& lastUpdate);

  /* Get a randon URL from the status file which points to the redundant whazzup files */
  QString getWhazzupUrlFromStatus(bool& gzipped) const;

  /* Get a randon voice server URL from the status file which points to the redundant whazzup files */
  QString getWhazzupVoiceUrlFromStatus() const;

  /* Get message which has to be shown on application start */
  const QString& getMessageFromStatus();

  /*  Time in minutes to wait before allowing manual Atis refresh by way of web page interface */
  int getAtisAllowMinutesFromWhazzup() const;

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

  /* Do any schema updates if needed */
  void updateSchema();

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
  static void fillFromClient(atools::fs::sc::SimConnectAircraft& ac, const atools::sql::SqlRecord& record);

  /* Get aircraft from table client by id and fill data into aircraft class. */
  void getClientAircraftById(atools::fs::sc::SimConnectAircraft& aircraft, int clientId);

  atools::sql::SqlRecord getClientRecordById(int clientId);

  /* Number of client aircraft in client table */
  int getNumClients() const;

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
