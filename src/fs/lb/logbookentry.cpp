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

#include "fs/lb/logbookentry.h"
#include "io/binarystream.h"
#include "sql/sqlutil.h"
#include "logging/loggingdefs.h"

#include <QString>
#include <QDateTime>

namespace atools {
namespace fs {
namespace lb {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::io::BinaryStream;

LogbookEntry::LogbookEntry(BinaryStream *bs, qint64 startpos, qint64 len, int entryNumber)
{
  // TODO keep dummy values for writing
  bs->readUShort(); // log entry, always 0
  bs->readUInt();

  // Read date
  year = checkNull(bs->readShort(), "year", entryNumber);
  bs->readUByte();
  month = checkNull(bs->readUByte(), "month", entryNumber);
  day = checkNull(bs->readUByte(), "day", entryNumber);
  hour = bs->readUByte();
  minute = bs->readUByte();
  second = bs->readUByte();

  // Airports
  airportFrom = bs->readString(4);
  airportTo = bs->readString(4);

  // Times
  totalTime = checkNull(bs->readFloat(), "total time", entryNumber);
  nightTime = bs->readFloat();
  instrumentTime = bs->readFloat();

  // Aircraft information
  aircraftType = static_cast<types::AircraftType>(bs->readUByte());
  flags = bs->readShort();
  // bool multimotor = (flags & 0x4000) != 0;
  int planeDescrLen = bs->readUByte();

  aircraftRegistration = bs->readString(10);
  aircraftDescription = bs->readString(planeDescrLen);

  // Get subrecords
  while(bs->tellg() < startpos + len)
  {
    types::RecordSubType subtype = static_cast<types::RecordSubType>(bs->readUByte());

    if(subtype == types::SUBRECORD_PLANE_DESCRIPTION)
    {
      // Never seen this one

      qDebug() << "Found subrecord PLANE_DESCRIPTION";
      /*short subFlags =*/ bs->readUShort();
      int subPlaneDescLen = bs->readUByte();
      /*QString subPlaneReg =*/ bs->readString(10);
      /*QString subPlaneDesc =*/ bs->readString(subPlaneDescLen);
      // bool subMultimotor = (subFlags & 0x4000) != 0;
    }
    else if(subtype == types::SUBRECORD_AIRPORT_LIST)
    {
      // Intermediate destinations
      qDebug() << "Found subrecord AIRPORT_LIST";
      bs->readUByte(); // airportListLen
      bs->readUShort(); // airports landing tables, always 0
      int nap = bs->readUShort();
      for(int i = 0; i < nap; i++)
      {
        QString subAp = bs->readString(4);
        int landings = bs->readUByte();

        airportVisits.push_back(AirportVisit(subAp, landings));
      }
    }
    else if(subtype == types::SUBRECORD_DESCRIPTION)
    {
      // Flight comment
      qDebug() << "Found subrecord DESCRIPTION";
      int descrLen = bs->readUByte() - 4;
      bs->readUShort(); // flight description, always 0
      int i = 0;
      char b = 0;
      while((i < descrLen) && ((b = bs->readByte()) != 5))
      {
        description.push_back(QChar::fromLatin1(b));
        i++;
      }

      if(b == 5)
      {
        qWarning() << "Found unexpected record 5 at" << bs->tellg() << "within record 4";
        bs->seekg(-1);
        break;
      }
    }
    else
    {
      qWarning() << "Found unknown record" << subtype << "at" << bs->tellg() << "within subrecords";
      bs->seekg(-1);
      break;
    }
  }
}

QVariant LogbookEntry::visitsToString() const
{
  QString retval;
  for(AirportVisit v : airportVisits)
  {
    if(!retval.isEmpty())
      retval += ", ";
    retval += v.getAirport() + "/" + QString::number(v.getLandings());
  }
  if(!retval.isEmpty())
    return QVariant(retval);
  else
    return QVariant(QVariant::String);
}

void LogbookEntry::fillEntryStatement(SqlQuery& stmt, int entryNumber)
{
  stmt.bindValue(":airport_from_icao", airportFrom);
  stmt.bindValue(":airport_from_name", QVariant(QVariant::String));
  stmt.bindValue(":airport_from_city", QVariant(QVariant::String));
  stmt.bindValue(":airport_from_state", QVariant(QVariant::String));
  stmt.bindValue(":airport_from_country", QVariant(QVariant::String));
  stmt.bindValue(":airport_to_icao", airportTo);
  stmt.bindValue(":airport_to_name", QVariant(QVariant::String));
  stmt.bindValue(":airport_to_city", QVariant(QVariant::String));
  stmt.bindValue(":airport_to_state", QVariant(QVariant::String));
  stmt.bindValue(":airport_to_country", QVariant(QVariant::String));
  stmt.bindValue(":distance", QVariant(QVariant::Double));

  stmt.bindValue(":airport_from_icao", airportFrom);
  stmt.bindValue(":airport_to_icao", airportTo);
  stmt.bindValue(":description", description);
  stmt.bindValue(":total_time", totalTime);
  stmt.bindValue(":night_time", nightTime);
  stmt.bindValue(":instrument_time", instrumentTime);
  stmt.bindValue(":aircraft_reg", aircraftRegistration);
  stmt.bindValue(":aircraft_descr", aircraftDescription);
  stmt.bindValue(":aircraft_type", aircraftType);
  stmt.bindValue(":aircraft_flags", flags);

  // Use a string to descripte all intermediate destinations
  stmt.bindValue(":visits", visitsToString());

  stmt.bindValue(":startdate", QVariant(QVariant::String));

  QDate date(year, month, day);
  if(date.isValid())
  {
    QTime time(hour, minute, second);
    if(time.isValid())
    {
      QDateTime dateTime(date, time);
      QDateTime dateTimeFixed = dateTime.addSecs(dateTime.offsetFromUtc());
      if(dateTime.isValid())
        stmt.bindValue(":startdate", dateTimeFixed.toTime_t());
      else
        stmt.bindValue(":startdate", QVariant(0));
    }
    else
      qWarning() << "time is not valid for entry" << entryNumber;
  }
  else
    qWarning() << "date is not valid for entry" << entryNumber;
}

SqlQuery LogbookEntry::prepareEntryStatement(SqlDatabase *db)
{
  SqlQuery q(db);
  q.prepare(SqlUtil(db).buildInsertStatement("logbook"));
  return q;
}

void LogbookEntry::fillVisitStatement(SqlQuery& stmt, int visitIndex)
{
  stmt.bindValue(":airport", airportVisits.at(visitIndex).getAirport());
  stmt.bindValue(":landings", airportVisits.at(visitIndex).getLandings());
}

SqlQuery LogbookEntry::prepareVisitStatement(SqlDatabase *db)
{
  SqlQuery q(db);
  q.prepare(SqlUtil(db).buildInsertStatement("logbook_visits"));
  return q;
}

} // namespace lb
} // namespace fs
} // namespace atools
