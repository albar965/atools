/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include <QDebug>
#include <QString>
#include <QDateTime>

namespace atools {
namespace fs {
namespace lb {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::io::BinaryStream;

LogbookEntry::LogbookEntry(BinaryStream *bs)
  : stream(bs)
{

}

void LogbookEntry::read(qint64 startpos, qint64 len, int entryNumber)
{
  reset();

  // TODO keep dummy values for writing
  stream->readUShort(); // log entry, always 0
  stream->readUInt();

  // Read date
  year = checkNull(stream->readUShort(), "year", entryNumber);
  stream->readUByte();
  month = checkNull(stream->readUByte(), "month", entryNumber);
  day = checkNull(stream->readUByte(), "day", entryNumber);
  hour = stream->readUByte();
  minute = stream->readUByte();
  second = stream->readUByte();

  // Set invalid date
  dateTime = QDateTime();

  QDate date(year, month, day);
  if(date.isValid())
  {
    QTime time(hour, minute, second);
    if(time.isValid())
      dateTime = QDateTime(date, time, Qt::UTC);
    else
      qWarning() << "time is not valid for entry" << entryNumber;
  }
  else
    qWarning() << "date is not valid for entry" << entryNumber;

  // Airports
  airportFrom = stream->readString(4);
  airportTo = stream->readString(4);

  // Times
  totalTime = checkNull(stream->readFloat(), "total time", entryNumber);
  nightTime = stream->readFloat();
  instrumentTime = stream->readFloat();

  // Aircraft information
  aircraftType = static_cast<types::AircraftType>(stream->readUByte());
  flags = stream->readUShort();
  // bool multimotor = (flags & 0x4000) != 0;
  int planeDescrLen = stream->readUByte();

  aircraftRegistration = stream->readString(10);
  aircraftDescription = stream->readString(planeDescrLen);

  // Get subrecords
  while(stream->tellg() < startpos + len)
  {
    types::RecordSubType subtype = static_cast<types::RecordSubType>(stream->readUByte());

    if(subtype == types::SUBRECORD_PLANE_DESCRIPTION)
    {
      // Never seen this one

      qDebug() << "Found subrecord PLANE_DESCRIPTION";
      /*short subFlags =*/ stream->readUShort();
      int subPlaneDescLen = stream->readUByte();
      /*QString subPlaneReg =*/ stream->readString(10);
      /*QString subPlaneDesc =*/ stream->readString(subPlaneDescLen);
      // bool subMultimotor = (subFlags & 0x4000) != 0;
    }
    else if(subtype == types::SUBRECORD_AIRPORT_LIST)
    {
      // Intermediate destinations
      qDebug() << "Found subrecord AIRPORT_LIST";
      stream->readUByte(); // airportListLen
      stream->readUShort(); // airports landing tables, always 0
      int nap = stream->readUShort();
      for(int i = 0; i < nap; i++)
      {
        QString subAp = stream->readString(4);
        int landings = stream->readUByte();

        airportVisits.append(AirportVisit(subAp, landings));
      }
    }
    else if(subtype == types::SUBRECORD_DESCRIPTION)
    {
      // Flight comment
      qDebug() << "Found subrecord DESCRIPTION";
      int descrLen = stream->readUByte() - 4;
      stream->readUShort(); // flight description, always 0
      int i = 0;
      char b = 0;
      while((i < descrLen) && ((b = stream->readByte()) != 5))
      {
        description.append(QChar::fromLatin1(b));
        i++;
      }

      if(b == 5)
      {
        qWarning() << "Found unexpected record 5 at" << stream->tellg() << "within record 4";
        stream->seekg(-1);
        break;
      }
    }
    else
    {
      qWarning() << "Found unknown record" << subtype << "at" << stream->tellg() << "within subrecords";
      stream->seekg(-1);
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

void LogbookEntry::fillEntryStatement(SqlQuery& stmt)
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
  // Store only multi engine since this is the only available flag
  stmt.bindValue(":aircraft_flags", flags & types::AIRCRAFT_FLAG_MULTIMOTOR);

  // Use a string to describe all intermediate destinations
  stmt.bindValue(":visits", visitsToString());

  stmt.bindValue(":startdate", QVariant(QVariant::String));

  if(dateTime.isValid())
    stmt.bindValue(":startdate", dateTime.toTime_t());
  else
    stmt.bindValue(":startdate", QVariant(0));
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

void LogbookEntry::reset()
{
  year = 0;
  month = 0;
  day = 0;
  hour = 0;
  minute = 0;
  second = 0;

  airportFrom.clear();
  airportTo.clear();
  description.clear();

  totalTime = 0.f;
  nightTime = 0.f;
  instrumentTime = 0.f;

  aircraftType = types::AIRCRAFT_UNKNOWN;

  flags = 0;
  aircraftRegistration.clear();
  aircraftDescription.clear();

  airportVisits.clear();
}

} // namespace lb
} // namespace fs
} // namespace atools
