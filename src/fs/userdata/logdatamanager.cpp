/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/userdata/logdatamanager.h"

#include "sql/sqlutil.h"
#include "sql/sqlexport.h"
#include "sql/sqltransaction.h"
#include "sql/sqldatabase.h"
#include "util/csvreader.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "exception.h"

#include <QDateTime>
#include <QDir>

namespace atools {
namespace fs {
namespace userdata {

using atools::geo::Pos;
using atools::sql::SqlUtil;
using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlExport;
using atools::sql::SqlRecord;
using atools::sql::SqlTransaction;

namespace csv {
/* Column indexes in CSV format */
enum Index
{
  // LOGBOOK_ID,- not in CSV
  AIRCRAFT_NAME,
  AIRCRAFT_TYPE,
  AIRCRAFT_REGISTRATION,
  FLIGHTPLAN_NUMBER,
  FLIGHTPLAN_CRUISE_ALTITUDE,
  FLIGHTPLAN_FILE,
  PERFORMANCE_FILE,
  BLOCK_FUEL,
  TRIP_FUEL,
  USED_FUEL,
  IS_JETFUEL,
  GROSSWEIGHT,
  DISTANCE,
  DISTANCE_FLOWN,
  DEPARTURE_IDENT,
  DEPARTURE_NAME,
  DEPARTURE_RUNWAY,
  DEPARTURE_LONX,
  DEPARTURE_LATY,
  DEPARTURE_ALT,
  DEPARTURE_TIME,
  DEPARTURE_TIME_SIM,
  DESTINATION_IDENT,
  DESTINATION_NAME,
  DESTINATION_RUNWAY,
  DESTINATION_LONX,
  DESTINATION_LATY,
  DESTINATION_ALT,
  DESTINATION_TIME,
  DESTINATION_TIME_SIM,
  ROUTE_STRING,
  SIMULATOR,
  DESCRIPTION
  // PLAN_GEOMETRY,
  // TRAIL_GEOMETRY
};

}

LogdataManager::LogdataManager(sql::SqlDatabase *sqlDb)
  : DataManagerBase(sqlDb, "logbook", "logbook_id",
                    ":/atools/resources/sql/fs/logbook/create_logbook_schema.sql",
                    ":/atools/resources/sql/fs/logbook/drop_logbook_schema.sql",
                    "little_navmap_logbook_backup.csv")
{

}

LogdataManager::~LogdataManager()
{

}

int LogdataManager::importCsv(const QString& filepath)
{
  SqlTransaction transaction(db);

  // Autogenerate id - exclude logbook_id from insert
  SqlQuery insertQuery(db);
  insertQuery.prepare(SqlUtil(db).buildInsertStatement(tableName, QString(), {idColumnName}, true));

  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    atools::util::CsvReader reader;

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      QString line = stream.readLine();

      if(lineNum == 0)
      {
        lineNum++;
        // Ignore header
        continue;
      }

      // Skip empty lines but add them if within an escaped field
      if(line.isEmpty() && !reader.isInEscape())
        continue;

      reader.readCsvLine(line);
      if(reader.isInEscape())
        // Still in an escaped line so continue to read unchanged until " shows the end of the field
        continue;

      const QStringList& values = reader.getValues();

      // Aircraft ===============================================================
      insertQuery.bindValue(":aircraft_name", at(values, csv::AIRCRAFT_NAME));
      insertQuery.bindValue(":aircraft_type", at(values, csv::AIRCRAFT_TYPE));
      insertQuery.bindValue(":aircraft_registration", at(values, csv::AIRCRAFT_REGISTRATION));

      // Flightplan ===============================================================
      insertQuery.bindValue(":flightplan_number", at(values, csv::FLIGHTPLAN_NUMBER));
      if(!at(values, csv::FLIGHTPLAN_CRUISE_ALTITUDE).isEmpty())
        insertQuery.bindValue(":flightplan_cruise_altitude", atFloat(values, csv::FLIGHTPLAN_CRUISE_ALTITUDE, true));
      insertQuery.bindValue(":flightplan_file", at(values, csv::FLIGHTPLAN_FILE));

      // Trip ===============================================================
      insertQuery.bindValue(":performance_file", at(values, csv::PERFORMANCE_FILE));
      if(!at(values, csv::BLOCK_FUEL).isEmpty())
        insertQuery.bindValue(":block_fuel", atFloat(values, csv::BLOCK_FUEL, true));
      if(!at(values, csv::TRIP_FUEL).isEmpty())
        insertQuery.bindValue(":trip_fuel", atFloat(values, csv::TRIP_FUEL, true));
      if(!at(values, csv::USED_FUEL).isEmpty())
        insertQuery.bindValue(":used_fuel", atFloat(values, csv::USED_FUEL, true));
      if(!at(values, csv::IS_JETFUEL).isEmpty())
        insertQuery.bindValue(":is_jetfuel", atInt(values, csv::IS_JETFUEL, true));
      if(!at(values, csv::GROSSWEIGHT).isEmpty())
        insertQuery.bindValue(":grossweight", atFloat(values, csv::GROSSWEIGHT, true));
      if(!at(values, csv::DISTANCE).isEmpty())
        insertQuery.bindValue(":distance", atFloat(values, csv::DISTANCE, true));
      if(!at(values, csv::DISTANCE_FLOWN).isEmpty())
        insertQuery.bindValue(":distance_flown", atFloat(values, csv::DISTANCE_FLOWN, true));

      // Departure ===============================================================
      insertQuery.bindValue(":departure_ident", at(values, csv::DEPARTURE_IDENT));
      insertQuery.bindValue(":departure_name", at(values, csv::DEPARTURE_NAME));
      insertQuery.bindValue(":departure_runway", at(values, csv::DEPARTURE_RUNWAY));

      if(!at(values, csv::DEPARTURE_LONX).isEmpty() && !at(values, csv::DEPARTURE_LATY).isEmpty())
      {
        Pos departPos = validateCoordinates(line, at(values, csv::DEPARTURE_LONX), at(values, csv::DEPARTURE_LATY));

        if(departPos.isValid())
        {
          insertQuery.bindValue(":departure_lonx", departPos.getLonX());
          insertQuery.bindValue(":departure_laty", departPos.getLatY());
        }
      }
      if(!at(values, csv::DEPARTURE_ALT).isEmpty())
        insertQuery.bindValue(":departure_alt", atFloat(values, csv::DEPARTURE_ALT, true));

      insertQuery.bindValue(":departure_time",
                            QDateTime::fromString(at(values, csv::DEPARTURE_TIME, true), Qt::ISODate));
      insertQuery.bindValue(":departure_time_sim",
                            QDateTime::fromString(at(values, csv::DEPARTURE_TIME_SIM, true), Qt::ISODate));

      // Destination ===============================================================
      insertQuery.bindValue(":destination_ident", at(values, csv::DESTINATION_IDENT));
      insertQuery.bindValue(":destination_name", at(values, csv::DESTINATION_NAME));
      insertQuery.bindValue(":destination_runway", at(values, csv::DESTINATION_RUNWAY));

      if(!at(values, csv::DESTINATION_LONX).isEmpty() && !at(values, csv::DESTINATION_LATY).isEmpty())
      {
        Pos destPos = validateCoordinates(line, at(values, csv::DESTINATION_LONX), at(values, csv::DESTINATION_LATY));
        if(destPos.isValid())
        {
          insertQuery.bindValue(":destination_lonx", destPos.getLonX());
          insertQuery.bindValue(":destination_laty", destPos.getLatY());
        }
      }

      if(!at(values, csv::DESTINATION_ALT).isEmpty())
        insertQuery.bindValue(":destination_alt", atFloat(values, csv::DESTINATION_ALT, true));

      insertQuery.bindValue(":destination_time",
                            QDateTime::fromString(at(values, csv::DESTINATION_TIME, true), Qt::ISODate));
      insertQuery.bindValue(":destination_time_sim",
                            QDateTime::fromString(at(values, csv::DESTINATION_TIME_SIM, true), Qt::ISODate));

      // Other ===============================================================
      insertQuery.bindValue(":route_string", at(values, csv::ROUTE_STRING));
      insertQuery.bindValue(":simulator", at(values, csv::SIMULATOR));
      insertQuery.bindValue(":description", at(values, csv::DESCRIPTION));

      // Geometry ===============================================================
      // insertQuery.bindValue(":plan_geometry", at(values, csv::PLAN_GEOMETRY));
      // insertQuery.bindValue(":trail_geometry", at(values, csv::TRAIL_GEOMETRY));

      // Fill null fields with empty strings to avoid issues when searching
      fixEmptyFields(insertQuery);

      insertQuery.exec();

      // Reset unassigned fields to null
      insertQuery.clearBoundValues();

      lineNum++;
      numImported++;
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));

  transaction.commit();
  return numImported;
}

// https://www.x-plane.com/manuals/desktop/index.html#keepingalogbook
// Each time an aircraft is flown in X-Plane, the program logs the flight time in a digital logbook.
// By default, X‑Plane creates a text file called “X-Plane Pilot.txt” in the ‘X-Plane 11/Output/logbooks directory’.
// Inside this text file are the following details of previous flights:
// 0	 Dates of flights
// 1	 Tail numbers of aircraft
// 2	 Aircraft types
// 3	 Airports of departure and arrival
// 4	 Number of landings
// 5	 Duration of flights
// 6	 Time spent flying cross-country, in IFR conditions, and at night
// 7	 Total time of all flights
//
// I
// 1 Version
// 2 170722    EDFE    EDFE   2   0.3   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170722    EDXW    EDXW   1   0.2   0.0   0.0   0.2   N45XS  Baron_58
// 2 170724    PHNL    PHNL   0   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170724    PHNL    PHNL   1   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170724    KBFI    KBFI   0   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// ...
// 2 190612    EDXW    EDXW   0   0.2   0.0   0.0   0.0    SF34
// 2 190612    EDXW    EDXW   2   0.2   0.0   0.0   0.0  G-LSCL  SF34
// 2 190612    EDXW    EDXW   2   0.2   0.0   0.0   0.0  Car_690B_TurboCommander
// 2 190620    CYPS    CYPS   0   0.1   0.1   0.0   0.0  N410LT  E1000G
// 2 190620    CYPS     BQ8   3   0.5   0.0   0.0   0.0  C-JEFF  E1000G
// 2 190620    FHAW    FHAW   0   0.1   0.0   0.0   0.0  N7779E  Car_B1900D
// 99
int LogdataManager::importXplane(const QString& filepath,
                                 const std::function<void(atools::geo::Pos& pos, QString& name,
                                                          const QString& ident)>& fetchAirport)
{
  using atools::at;
  using atools::atFloat;
  using atools::atInt;

  enum XPlaneLogbookIdx
  {
    PREFIX,
    DATE,
    DEPARTURE,
    DESTINATION,
    NUM_LANDINGS,
    TIME,
    TIME_CROSS_COUNTRY,
    TIME_IFR,
    TIME_NIGHT,
    TAIL_NUMBER,
    AIRCRAFT_TYPE
  };

  SqlTransaction transaction(db);

  // Autogenerate id
  SqlQuery insertQuery(db);
  insertQuery.prepare(SqlUtil(db).buildInsertStatement(tableName, QString(), {idColumnName}, true));

  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QString filename = QFileInfo(filepath).fileName();

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      QString readline = stream.readLine();
      if(readline == "99") // Check for end of file marker
        break;

      QStringList line = readline.simplified().split(" ");

      if(line.size() >= 9) // Reg and type might be omitted
      {
        // 2 190620    FHAW    FHAW   0   0.1   0.0   0.0   0.0  N7779E  Car_B1900D
        if(line.at(PREFIX) != "2")
          qWarning() << Q_FUNC_INFO << "Unknown prefix" << line.at(PREFIX) << "at line" << lineNum;

        // Time ========================
        int travelTimeSecs = atools::roundToInt(atFloat(line, TIME, true) * 3600.f);
        QDateTime departureTime = QDateTime::fromString("20" + at(line, DATE), "yyyyMMdd");
        QDateTime destinationTime = departureTime.addSecs(travelTimeSecs);

        // Resolve departure and destination ================================
        QString departure = at(line, DEPARTURE), departureName;
        atools::geo::Pos departurePos;

        if(departure == "BIDV")
          qDebug() << Q_FUNC_INFO;

        // Get name and coordinates from database
        fetchAirport(departurePos, departureName, departure);

        // Departure =====================================================
        insertQuery.bindValue(":departure_ident", departure);
        insertQuery.bindValue(":departure_name", departureName);

        if(departurePos.isValid())
        {
          // Leave position null, otherwise
          insertQuery.bindValue(":departure_lonx", departurePos.getLonX());
          insertQuery.bindValue(":departure_laty", departurePos.getLatY());
          insertQuery.bindValue(":departure_alt", departurePos.getAltitude());
        }

        insertQuery.bindValue(":departure_time_sim", departureTime);
        insertQuery.bindValue(":departure_time", departureTime);

        // Destination =====================================================
        // Get name and coordinates from database
        QString destination = at(line, DESTINATION), destinationName;
        atools::geo::Pos destinationPos;
        fetchAirport(destinationPos, destinationName, destination);

        insertQuery.bindValue(":destination_ident", destination);
        insertQuery.bindValue(":destination_name", destinationName);

        if(destinationPos.isValid())
        {
          insertQuery.bindValue(":destination_lonx", destinationPos.getLonX());
          insertQuery.bindValue(":destination_laty", destinationPos.getLatY());
          insertQuery.bindValue(":destination_alt", destinationPos.getAltitude());
        }
        insertQuery.bindValue(":destination_time_sim", destinationTime);
        insertQuery.bindValue(":destination_time", destinationTime);

        // Aircraft ====================================================
        if(TAIL_NUMBER < line.size())
        {
          QString tailNum = at(line, TAIL_NUMBER);
          insertQuery.bindValue(":aircraft_registration", tailNum.replace("_", " "));
        }

        if(AIRCRAFT_TYPE < line.size())
        {
          QString aircraftType = at(line, AIRCRAFT_TYPE);
          insertQuery.bindValue(":aircraft_type", aircraftType.replace("_", " "));
        }

        // ===================================================================
        if(departurePos.isValid() && destinationPos.isValid())
          insertQuery.bindValue(":distance", atools::geo::meterToNm(departurePos.distanceMeterTo(destinationPos)));

        insertQuery.bindValue(":simulator", "X-Plane 11");

        // Description ===================================================================
        /*: The text "Imported from X-Plane logbook" has to match the one in LogdataController::importXplane */
        QString description(tr("Imported from X-Plane logbook %1\n"
                               "Number of landings: %2\n"
                               "Cross country time: %3\n"
                               "IFR time: %4\n"
                               "Night time: %5").
                            arg(filename).
                            arg(atInt(line, NUM_LANDINGS, true)).
                            arg(atFloat(line, TIME_CROSS_COUNTRY, true), 0, 'f', 1).
                            arg(atFloat(line, TIME_IFR, true), 0, 'f', 1).
                            arg(atFloat(line, TIME_NIGHT, true), 0, 'f', 1));
        insertQuery.bindValue(":description", description);

        // Fill null fields with empty strings to avoid issues when searching
        fixEmptyFields(insertQuery);

        insertQuery.exec();
        insertQuery.clearBoundValues();
      } // if(line.size() >= 10)

      lineNum++;
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));

  transaction.commit();
  return numImported;

}

int LogdataManager::exportCsv(const QString& filepath)
{
  int exported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setRealNumberPrecision(5);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    SqlUtil util(db);
    SqlQuery query(util.buildSelectStatement(tableName, util.buildColumnList(tableName, {idColumnName})), db);
    SqlExport sqlExport;
    exported = sqlExport.printResultSet(query, out);
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open backup file %1. Reason: %2 (%3)").
                            arg(filepath).arg(file.errorString()).arg(file.error()));
  return exported;
}

void LogdataManager::updateSchema()
{
  if(!db->record(tableName).contains("route_string"))
  {
    qDebug() << Q_FUNC_INFO;

    SqlTransaction transaction(db);
    // Add missing column
    db->exec("alter table " + tableName + " add column route_string varchar(1024)");
    transaction.commit();
  }
}

void LogdataManager::getFlightStatsTime(QDateTime& earliest, QDateTime& latest, QDateTime& earliestSim,
                                        QDateTime& latestSim)
{
  SqlQuery query("select min(departure_time), max(departure_time), "
                 "min(departure_time_sim), max(departure_time_sim) from " + tableName, db);
  query.exec();
  if(query.next())
  {
    earliest = query.valueDateTime(0);
    latest = query.valueDateTime(1);
    earliestSim = query.valueDateTime(2);
    latestSim = query.valueDateTime(3);
  }
}

void LogdataManager::getFlightStatsDistance(float& distTotal, float& distMax, float& distAverage)
{
  SqlQuery query("select sum(distance), max(distance), avg(distance) from " + tableName, db);
  query.exec();
  if(query.next())
  {
    distTotal = query.valueFloat(0);
    distMax = query.valueFloat(1);
    distAverage = query.valueFloat(2);
  }
}

void LogdataManager::getFlightStatsAirports(int& numDepartAirports, int& numDestAirports)
{
  SqlQuery query("select count(distinct departure_ident), count(distinct destination_ident) from " + tableName, db);
  query.exec();
  if(query.next())
  {
    numDepartAirports = query.valueInt(0);
    numDestAirports = query.valueInt(1);
  }
}

void LogdataManager::getFlightStatsAircraft(int& numTypes, int& numRegistrations, int& numNames, int& numSimulators)
{
  SqlQuery query("select count(distinct aircraft_type), count(distinct aircraft_registration), "
                 "count(distinct aircraft_name), count(distinct simulator) "
                 "from " + tableName, db);
  query.exec();
  if(query.next())
  {
    numTypes = query.valueInt(0);
    numRegistrations = query.valueInt(1);
    numNames = query.valueInt(2);
    numSimulators = query.valueInt(3);
  }
}

void LogdataManager::getFlightStatsSimulator(QVector<std::pair<int, QString> >& numSimulators)
{
  SqlQuery query("select count(1), simulator from " + tableName + " group by simulator order by count(1) desc", db);
  query.exec();
  while(query.next())
    numSimulators.append(std::make_pair(query.valueInt(0), query.valueStr(1)));
}

void LogdataManager::fixEmptyStrField(sql::SqlRecord& rec, const QString& name)
{
  if(rec.contains(name) && rec.isNull(name))
    rec.setValue(name, "");
}

void LogdataManager::fixEmptyStrField(sql::SqlQuery& query, const QString& name)
{
  if(query.boundValue(name, true).isNull())
    query.bindValue(name, "");
}

void LogdataManager::fixEmptyFields(sql::SqlRecord& rec)
{
  if(rec.contains("distance") && rec.isNull("distance"))
    rec.setValue("distance", 0.f);

  fixEmptyStrField(rec, "aircraft_name");
  fixEmptyStrField(rec, "aircraft_type");
  fixEmptyStrField(rec, "aircraft_registration");
  fixEmptyStrField(rec, "route_string");
  fixEmptyStrField(rec, "description");
  fixEmptyStrField(rec, "simulator");
  fixEmptyStrField(rec, "departure_ident");
  fixEmptyStrField(rec, "destination_ident");
}

void LogdataManager::fixEmptyFields(sql::SqlQuery& query)
{
  if(query.boundValue(":distance", true).isNull())
    query.bindValue(":distance", 0.f);

  fixEmptyStrField(query, ":aircraft_name");
  fixEmptyStrField(query, ":aircraft_type");
  fixEmptyStrField(query, ":aircraft_registration");
  fixEmptyStrField(query, ":route_string");
  fixEmptyStrField(query, ":description");
  fixEmptyStrField(query, ":simulator");
  fixEmptyStrField(query, ":departure_ident");
  fixEmptyStrField(query, ":destination_ident");
}

void LogdataManager::getFlightStatsTripTime(float& timeMaximum, float& timeAverage, float& timeMaximumSim,
                                            float& timeAverageSim)
{
  SqlQuery query("select max(time_real), avg(time_real), max(time_sim), avg(time_sim) from (select "
                 "strftime('%s', destination_time) - strftime('%s', departure_time) as time_real, "
                 "strftime('%s', destination_time_sim) - strftime('%s', departure_time_sim) as time_sim "
                 "from " + tableName + ")", db);
  query.exec();
  if(query.next())
  {
    timeMaximum = query.valueFloat(0) / 3600.f;
    timeAverage = query.valueFloat(1) / 3600.f;
    timeMaximumSim = query.valueFloat(2) / 3600.f;
    timeAverageSim = query.valueFloat(3) / 3600.f;
  }
}

} // namespace userdata
} // namespace fs
} // namespace atools
