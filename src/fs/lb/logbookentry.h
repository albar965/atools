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

#ifndef ATOOLS_FS_LB_LOGBOOKENTRY_H
#define ATOOLS_FS_LB_LOGBOOKENTRY_H

#include "fs/lb/types.h"
#include "logging/loggingdefs.h"
#include "sql/sqlquery.h"

#include <QDateTime>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}
namespace io {
class BinaryStream;
}

namespace fs {
namespace lb {

/*
 * Reads a single logbook entry from the Logbook.BIN file.
 */
class LogbookEntry
{
public:
  /*
   *
   * @param bs Stream to read positioned after size and type fields
   * @param startpos Start position in the stream before reading size
   * and type fields
   * @param length of the entry
   * @return
   */
  LogbookEntry(atools::io::BinaryStream *bs);

  void read(qint64 startpos, qint64 len, int entryNumber);

  /* Prepare a query to insert rows into the logbook table */
  static atools::sql::SqlQuery prepareEntryStatement(atools::sql::SqlDatabase *db);

  /* Prepare a query to insert rows into the logbook_visits table */
  static atools::sql::SqlQuery prepareVisitStatement(atools::sql::SqlDatabase *db);

  /* Fill the prepared statement with values */
  void fillEntryStatement(atools::sql::SqlQuery& stmt);

  /* Fill the prepared statement with values */
  void fillVisitStatement(atools::sql::SqlQuery& stmt, int visitIndex);

  int getNumVisits() const
  {
    return airportVisits.size();
  }

  class AirportVisit
  {
public:
    AirportVisit()
    {
    }

    AirportVisit(QString ap, int l)
      : airport(ap), landings(l)
    {
    }

    const QString& getAirport() const
    {
      return airport;
    }

    int getLandings() const
    {
      return landings;
    }

private:
    QString airport;
    int landings = 0;
  };

  const QString& getAirportFrom() const
  {
    return airportFrom;
  }

  const QString& getAirportTo() const
  {
    return airportTo;
  }

  const QString& getDescription() const
  {
    return description;
  }

  const QString& getAircraftRegistration() const
  {
    return aircraftRegistration;
  }

  const QString& getAircraftDescription() const
  {
    return aircraftDescription;
  }

  types::AircraftType getAircraftType() const
  {
    return aircraftType;
  }

  QDateTime getDateTime() const
  {
    return dateTime;
  }

  float getTotalTimeMin() const
  {
    return totalTime;
  }

private:
  /* Print the entry to a stream or qdebug */
  template<typename T>
  friend  T& operator<<(T& out, const LogbookEntry& e);

  /* print a warning message, if the value is null */
  template<typename T>
  T checkNull(T value, const QString& msg, int entryNumber) const;

  unsigned short year;
  int month, day, hour, minute, second;
  QDateTime dateTime;
  QString airportFrom, airportTo, description;
  atools::fs::lb::types::AircraftType aircraftType;

  // All in decimal hours
  float totalTime, nightTime, instrumentTime;

  unsigned short flags;
  QString aircraftRegistration, aircraftDescription;

  QList<atools::fs::lb::LogbookEntry::AirportVisit> airportVisits;
  QVariant visitsToString() const;

  atools::io::BinaryStream *stream;
  void reset();

};

// ---------------------------------------------------------------------------

template<typename T>
T LogbookEntry::checkNull(T value, const QString& msg, int entryNumber) const
{
  if(value == 0)
  {
    if(msg.isEmpty())
      qWarning() << QString("Value is null. Entry %1").arg(entryNumber);
    else
      qWarning() << QString("Value for %1 is null. Entry %2").arg(msg).arg(entryNumber);
  }
  return value;
}

template<typename T>
T& operator<<(T& out, const LogbookEntry& e)
{
  out << "From " << e.getAirportFrom() << " to " << e.getAirportTo()
  << " reg " << e.getAircraftRegistration() << " description " << e.getAircraftDescription();
  return out;
}

} /* namespace lb */
} /* namespace fs */
} /* namespace atools */

#endif // ATOOLS_FS_LB_LOGBOOKENTRY_H
