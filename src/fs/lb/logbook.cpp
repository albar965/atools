/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/lb/logbook.h"
#include "io/binarystream.h"
#include "fs/lb/logbookentry.h"
#include "fs/lb/logbookentryfilter.h"
#include "fs/lb/types.h"
#include "sql/sqldatabase.h"
#include "geo/calculations.h"
#include "geo/pos.h"
#include "sql/sqlutil.h"
#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace lb {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::io::BinaryStream;

Logbook::Logbook(SqlDatabase *sqlDb, atools::fs::SimulatorType type)
  : db(sqlDb), sim(type)
{
}

void Logbook::read(QFile *file, const LogbookEntryFilter& filter, bool append)
{
  // TODO read also dummy entries to allow write back
  BinaryStream bs(file, QDataStream::LittleEndian);

  qint64 size = bs.getFileSize();
  int logbookId = 0, visitId = 0, entryNumber = 0, numEntriesInDb = 0, inserted = 0, filtered = 0;
  bool hasAirports = atools::sql::SqlUtil(db).hasTableAndRows("airport");

  SqlQuery entryStmt = LogbookEntry::prepareEntryStatement(db);
  SqlQuery visitStmt = LogbookEntry::prepareVisitStatement(db);

  SqlQuery airportStmt(db);
  if(hasAirports)
    airportStmt.prepare("select name, city, state, country, longitude, latitude "
                        "from airport where icao = :icao");

  SqlQuery countStmt(db);

  countStmt.exec("select count(*) from logbook where simulator_id = " + QString().setNum(sim));
  if(countStmt.next())
    numEntriesInDb = countStmt.value(0).toInt();

  // Calculate the next available logbook ID
  countStmt.exec("select max(logbook_id) from logbook where simulator_id = " + QString().setNum(sim));
  if(countStmt.next())
    logbookId = countStmt.value(0).toInt() + 1;

  // Calculate the next available visit ID
  countStmt.exec("select max(visit_id) from logbook_visits");
  if(countStmt.next())
    visitId = countStmt.value(0).toInt() + 1;

  qDebug() << "Found" << numEntriesInDb << "entries in database";

  LogbookEntry e(&bs);

  while(bs.tellg() < size)
  {
    qint64 startpos = bs.tellg();

    // Read header
    // TODO better error handling for invalid files
    types::RecordType type = static_cast<types::RecordType>(bs.readUByte());
    qint64 length = bs.readUByte();

    if(length == 0)
    {
      qWarning() << "Record length = 0 at entry" << entryNumber;
      break;
    }

    if(type == types::RECORD_LOGBOOK_ENTRY)
    {
      e.read(startpos, length, entryNumber);

      qDebug().nospace() << "Read entry number " << entryNumber << " (" << e << ")";

      // Add only new entries in append mode
      if(!append || entryNumber >= numEntriesInDb)
      {
        if(filter.canStore(e))
        {
          // Sets optional fields already to null
          e.fillEntryStatement(entryStmt);

          entryStmt.bindValue(":logbook_id", logbookId);
          entryStmt.bindValue(":simulator_id", sim);

          // select and add airport information if airport table is available
          if(hasAirports)
          {
            double startLon = 0., startLat = 0., destLon = 0., destLat = 0.;
            bool hasStartCoord = false, hasDestCoord = false;

            airportStmt.bindValue(":icao", e.getAirportFrom());
            airportStmt.exec();
            if(airportStmt.next())
            {
              entryStmt.bindValue(":airport_from_name", airportStmt.value("name").toString());
              entryStmt.bindValue(":airport_from_city", airportStmt.value("city").toString());
              entryStmt.bindValue(":airport_from_state", airportStmt.value("state").toString());
              entryStmt.bindValue(":airport_from_country", airportStmt.value("country").toString());

              if(!airportStmt.isNull("latitude") && !airportStmt.isNull("longitude"))
              {
                startLon = airportStmt.value("longitude").toDouble();
                startLat = airportStmt.value("latitude").toDouble();
                hasStartCoord = true;
              }
            }

            airportStmt.bindValue(":icao", e.getAirportTo());
            airportStmt.exec();
            if(airportStmt.next())
            {
              entryStmt.bindValue(":airport_to_name", airportStmt.value("name").toString());
              entryStmt.bindValue(":airport_to_city", airportStmt.value("city").toString());
              entryStmt.bindValue(":airport_to_state", airportStmt.value("state").toString());
              entryStmt.bindValue(":airport_to_country", airportStmt.value("country").toString());

              if(!airportStmt.isNull("latitude") && !airportStmt.isNull("longitude"))
              {
                destLon = airportStmt.value("longitude").toDouble();
                destLat = airportStmt.value("latitude").toDouble();
                hasDestCoord = true;
              }
            }

            // Store distance if there is a start and a destination airport
            if(hasStartCoord && hasDestCoord)
              entryStmt.bindValue(":distance", calcDist(startLon, startLat, destLon, destLat));
            else
              entryStmt.bindValue(":distance", QVariant(QVariant::Double));
          }

          entryStmt.exec();

          // Store all intermediate destinations in a separate table
          for(int i = 0; i < e.getNumVisits(); i++)
          {
            e.fillVisitStatement(visitStmt, i);
            visitStmt.bindValue(":visit_id", visitId);
            visitStmt.bindValue(":logbook_id", logbookId);
            visitStmt.bindValue(":simulator_id", sim);
            visitStmt.exec();
            visitId++;
          }

          inserted++;
        } // if(filter.canStore
        else
          filtered++;

        // Increment ids for read entries (all, filterd or not)
        logbookId++;
      } // if(!append ...

      entryNumber++;
    }
    else
    {
      qWarning() << "unknown type" << type << "at entry" << entryNumber;

      // Skip this unknown entry
      bs.seekg(length);
    }
  } // while

  qDebug() << "Read" << entryNumber << "entries. Inserted" << inserted << "entries and fitered out"
           << filtered << "entries.";

  db->commit();
  numLoaded = inserted;
}

double Logbook::calcDist(double startLon, double startLat, double destLon, double destLat) const
{
  return atools::geo::distanceInNm(atools::geo::Pos(startLon, startLat), atools::geo::Pos(destLon, destLat));
}

} // namespace lb
} // namespace fs
} // namespace atools
