/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_WHAZZUPTEXTPARSER_H
#define ATOOLS_FS_WHAZZUPTEXTPARSER_H

#include "geo/pos.h"
#include "fs/online/onlinetypes.h"

#include <QDateTime>
#include <QString>

class QTextStream;

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace online {

/*
 * Reads a "whazzup.txt" file and stores all found data in the database.
 * Schema has to be created before.
 *
 * Supported formats are the ones used by VATSIM and IVAO.
 *
 * Also reads new JSON formats.
 */
class WhazzupTextParser
{
public:
  WhazzupTextParser(atools::sql::SqlDatabase *sqlDb, bool verboseErrorReporting);
  virtual ~WhazzupTextParser();

  WhazzupTextParser(const WhazzupTextParser& other) = delete;
  WhazzupTextParser& operator=(const WhazzupTextParser& other) = delete;

  /* Read file content given in string and store results in database. Commit is executed when done.
   * Reads either "whazzup.txt" format or VATSIM JSON format depending on "streamFormat".
   * Returns true if the file was read and is more recent than lastUpdate. */
  bool read(QString file, atools::fs::online::Format streamFormat, const QDateTime& lastUpdate);

  /* Read VATSIM transceivers-data.json and stores map in this object. Call before calling "read". */
  void readTransceivers(const QString& file);

  /* Create all queries */
  void initQueries();

  /* Delete all queries */
  void deInitQueries();

  /* The last date and time this file has been updated. */
  QDateTime getLastUpdateTime() const
  {
    return updateTimestamp;
  }

  /* Time in minutes this file will be updated */
  int getReloadMinutes() const
  {
    return reload;
  }

  void reset();
  void resetForNewOptions();

  /* Set default circle radii for certain ATC types where visual range is unusable */
  void setAtcSize(const AtcSizeMap& sizeMap)
  {
    atcSizeMap = sizeMap;
  }

  /* Set a callback that tries to fetch geometry from the user airspace database.
   * Default circle will be used if this returns an empty byte array. */
  void setGeometryCallback(GeoCallbackType func)
  {
    geometryCallback = func;
  }

private:
  /* Read time from general section */
  QDateTime parseGeneralSection(const QStringList& line);

  /* Read ATC or client section */
  void parseSection(const QStringList& line, bool isAtc, bool prefile, bool isJson);

  /* Servers */
  void parseServersSection(const QStringList& line);

  /* Voice servers */
  void parseVoiceSection(const QStringList& line);

  /* Remove special characters from ATC text */
  QString convertAtisText(QString atis);

  /* Read datetime format */
  QDateTime parseDateTime(const QStringList& line, int index, bool jsonFormat);

  /* Fix UTF-8 name embedded in ANSI encoding in file */
  QString convertName(QString name, bool utf8);
  int semiPermanentId(QHash<QString, int>& idMap, int& curId, const QString& key);

  /* Read VATSIM JSON format and create a column list based on the whazzup.txt lists.
   * This is read by the delimited methods. */
  bool readInternalJson(const QString& file, const QDateTime& lastUpdate);
  bool readInternalDelimited(QTextStream& stream, const QDateTime& lastUpdate);
  void readPilotsJson(const QJsonArray& pilotsArr);
  void readControllersJson(const QJsonArray& controllersArr, bool observer);
  void readServersJson(const QJsonArray& serversArr, bool voice);
  void readPrefilesJson(const QJsonObject& obj);

  void readAtisJson(const QJsonObject& obj);

  /* Insert flight plan values into columns. Used for clients and prefile */
  void assignFlightplan(QStringList& columns, const QJsonObject& flightplanObj);

  QString curSection;
  atools::fs::online::Format format = atools::fs::online::UNKNOWN;

  /* Data format version */
  int version = 0;

  /* Time in minutes this file will be updated */
  int reload = 0;

  /* The last date and time this file has been updated. */
  QDateTime updateTimestamp;

  atools::sql::SqlDatabase *db;
  atools::sql::SqlQuery *clientInsertQuery = nullptr, *atcInsertQuery = nullptr, *serverInsertQuery = nullptr;

  // Assign row ids manually
  int curClientId = 1, curAtcId = 1;

  struct Transceiver
  {
    // In kHz
    QSet<int> frequency;
    atools::geo::Pos pos;
  };

  QMultiHash<QString, Transceiver> transceiverMap;

  // Keep a map of callsign to row id in the database to reuse the ids for the same centers and clients
  // This is to avoid id changes on every reload
  QHash<QString, int> clientIdMap, atcIdMap;
  AtcSizeMap atcSizeMap;

  // Report errors on warning channel
  bool error = false;

  GeoCallbackType geometryCallback;
  QStringList ivaoDefaultColumns, defaultColumns;
};

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WHAZZUPTEXTPARSER_H
