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

#include "fs/xp/airwaypostprocess.h"

#include "fs/navdatabaseoptions.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "exception.h"
#include "atools.h"

#include <QDebug>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

bool operator==(const AirwayPoint& seg1, const AirwayPoint& seg2)
{
  return seg1.ident == seg2.ident && seg1.region == seg2.region && seg1.type == seg2.type;
}

bool operator!=(const AirwayPoint& seg1, const AirwayPoint& seg2)
{
  return !(seg1 == seg2);
}

bool operator==(const AirwaySegment& seg1, const AirwaySegment& seg2)
{
  return seg1.next == seg2.next && seg1.prev == seg2.prev;
}

bool operator!=(const AirwaySegment& seg1, const AirwaySegment& seg2)
{
  return !(seg1 == seg2);
}

inline uint qHash(const AirwayPoint& seg)
{
  return qHash(seg.ident) ^ qHash(seg.region) ^ static_cast<uint>(seg.type);
}

inline uint qHash(const AirwaySegment& seg)
{
  return qHash(seg.prev) ^ qHash(seg.next);
}

/* Convert X-Plane type into database type */
QString convertType(AirwayPointType xptype)
{
  switch(xptype)
  {
    case atools::fs::xp::AW_NONE:
      return QString();

    case atools::fs::xp::AW_FIX:
      return "WN";

    case atools::fs::xp::AW_NDB:
      return "N";

    case atools::fs::xp::AW_VOR:
      return "V";
  }
  return QString();
}

/* X-Plane to database type */
QString convertAirwayType(AirwayType type)
{
  switch(type)
  {
    case atools::fs::xp::NONE:
      return QString();

    case atools::fs::xp::BOTH:
      return "B";

    case atools::fs::xp::VICTOR:
      return "V";

    case atools::fs::xp::JET:
      return "J";
  }
  return QString();
}

// ==================================================================

AirwayPostProcess::AirwayPostProcess(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                                     ProgressHandler *progress)
  : options(opts), db(sqlDb), progressHandler(progress)
{
}

AirwayPostProcess::~AirwayPostProcess()
{
}

bool AirwayPostProcess::postProcessEarthAirway()
{
  // A29
  // A123 - 1
  // A105 - 2
  // A126 -5
  // A125 -8
  // V16 -216
  // where name in ('V16')
  SqlQuery query("select name, type, direction, minimum_altitude, maximum_altitude, "
                 "previous_type, previous_ident, previous_region, "
                 "next_type, next_ident, next_region from airway_temp order by name", db);

  SqlQuery insert(db);
  insert.prepare(SqlUtil(db).buildInsertStatement("airway_point", QString(),
                                                  {"airway_point_id", "waypoint_id", "next_airport_ident",
                                                   "previous_airport_ident"}));

  QString currentAirway;
  AirwayType currentAirwayType = NONE;
  QList<AirwaySegment> segments;

  // Read duplets from temp table
  while(query.next())
  {
    QString airway = query.value("name").toString();
    AirwayType airwayType = static_cast<AirwayType>(query.value("type").toInt());

    if(currentAirway.isEmpty())
    {
      // First iteration
      currentAirway = airway;
      currentAirwayType = airwayType;
    }

    if(currentAirway != airway && !segments.isEmpty())
    {
      // Airway has changed - order and write all its segments
      writeSegments(segments, insert, currentAirway, currentAirwayType);

      segments.clear();
      currentAirway = airway;
      currentAirwayType = airwayType;
    }

    // Add a segment from the database
    AirwaySegment segment;
    segment.minAlt = query.value("minimum_altitude").toInt();
    segment.maxAlt = query.value("maximum_altitude").toInt();
    segment.next.ident = query.value("next_ident").toString();
    segment.next.region = query.value("next_region").toString();
    segment.next.type = static_cast<AirwayPointType>(query.value("next_type").toInt());
    segment.prev.ident = query.value("previous_ident").toString();
    segment.prev.region = query.value("previous_region").toString();
    segment.prev.type = static_cast<AirwayPointType>(query.value("previous_type").toInt());
    segment.dir = atools::strToChar(query.value("direction").toString());

    segments.insert(segments.size(), segment);
  }

  // Write the last airway
  if(!segments.isEmpty())
    writeSegments(segments, insert, currentAirway, currentAirwayType);

  return false;
}

void AirwayPostProcess::writeSegments(QList<AirwaySegment>& segments, SqlQuery& insert, const QString& name,
                                      AirwayType type)
{
  // if(name != "UJ20" && name != "UJ3")
  // return;

  // Create a list ordered by next waypoint id
  QVector<AirwaySegment> segsByNext(segments.toVector());
  std::sort(segsByNext.begin(), segsByNext.end(), nextOrderFunc);

  // Create a list ordered by previous waypoint id
  QVector<AirwaySegment> segsByPrev(segments.toVector());
  std::sort(segsByPrev.begin(), segsByPrev.end(), prevOrderFunc);

  // List of finished segments with original and reversed from/to
  QSet<AirwaySegment> done;

  // for(const Segment& s : segsByNext)
  // qDebug() << "BYNEXT" << s.prev.ident << s.prev.region << s.prev.type
  // << s.next.ident << s.next.region << s.next.type;
  // for(const Segment& s : segsByPrev)
  // qDebug() << "BYPREV" << s.prev.ident << s.prev.region << s.prev.type
  // << s.next.ident << s.next.region << s.next.type;

  // Iterate over all segments of this airway
  while(!segments.isEmpty())
  {
    // Create an airway fragment
    QVector<AirwaySegment> found;
    QVector<AirwaySegment> sortedSegments;

    // Insert start segment
    sortedSegments.append(segments.first());

    // Start segment is finished here - add from/to and to/from to simplify search
    done.insert(segments.first());
    done.insert(segments.first().reversed());
    segments.removeFirst();

    bool foundPrev = true, foundNext = true;
    while(foundNext || foundPrev)
    {
      // Find next for last segment
      const AirwaySegment& last = sortedSegments.last();
      if(findSegment(found, done, segsByPrev, last.next, last.prev, true))
      {
        // Found segment is in correct order
        segments.removeOne(found.first());
        sortedSegments.append(found.first());
        foundNext = true;
      }
      else if(findSegment(found, done, segsByNext, last.next, last.prev, false))
      {
        // Found segment is in reversed order
        segments.removeOne(found.first());
        sortedSegments.append(found.first().reversed());
        foundNext = true;
      }
      else
        foundNext = false;

      // Find previous for first segment
      const AirwaySegment& first = sortedSegments.first();
      if(findSegment(found, done, segsByNext, first.prev, first.next, false))
      {
        // Found segment is in correct order
        segments.removeOne(found.first());
        sortedSegments.prepend(found.first());
        foundPrev = true;
      }
      else if(findSegment(found, done, segsByPrev, first.prev, first.next, true))
      {
        // Found segment is in reversed order
        segments.removeOne(found.first());
        sortedSegments.prepend(found.first().reversed());
        foundPrev = true;
      }
      else
        foundPrev = false;
    }

    // qDebug() << "-------";
    // for(const Segment& s: sortedSegments)
    // qDebug() << "SORTED" << s.prev.ident << s.prev.region << s.prev.type
    // << s.next.ident << s.next.region << s.next.type;
    // qDebug() << "-------";

    // Write the from/via/to triplets now
    for(int i = 0; i < sortedSegments.size(); i++)
    {
      // use value method to get default constructed objects if index is invalid
      // 1 -> 2
      AirwaySegment prev12 = sortedSegments.value(i - 1);
      // 2 -> 3
      AirwaySegment mid23 = sortedSegments.value(i);
      // 3 -> 4
      AirwaySegment next34 = sortedSegments.value(i + 1);

      // Write in order 1 -> 2 -> 3
      // qDebug() << "1 -> 2 -> 3 ###################################################";
      // qDebug() << name << prev12.prev.ident << ">" << QChar(prev12.dir) << ">" << prev12.next.ident
      // << mid23.prev.ident << ">" << QChar(mid23.dir) << ">" << mid23.next.ident;

      if(i == 0) // Avoid overlapping/duplicates
        writeSegment(insert, name, type, prev12, mid23);

      // Write in order 2 -> 3 -> 4
      // qDebug() << "2 -> 3 -> 4 ######";
      // qDebug() << name << mid23.prev.ident << ">" << QChar(mid23.dir) << ">" << mid23.next.ident
      // << next34.prev.ident << ">" << QChar(next34.dir) << ">" << next34.next.ident;
      writeSegment(insert, name, type, mid23, next34);
    }
  }
}

void AirwayPostProcess::writeSegment(SqlQuery& insert, const QString& name, AirwayType type,
                                     const AirwaySegment& prevSeg, const AirwaySegment& nextSeg)
{
  insert.clearBoundValues();

  insert.bindValue(":name", name);
  insert.bindValue(":type", convertAirwayType(type));
  insert.bindValue(":mid_type", convertType(prevSeg.next.type));
  insert.bindValue(":mid_ident", prevSeg.next.ident);
  insert.bindValue(":mid_region", prevSeg.next.region);

  if(!prevSeg.prev.ident.isEmpty())
  {
    insert.bindValue(":previous_type", convertType(prevSeg.prev.type));
    insert.bindValue(":previous_ident", prevSeg.prev.ident);
    insert.bindValue(":previous_region", prevSeg.prev.region);
    insert.bindValue(":previous_minimum_altitude", prevSeg.minAlt * 100);
    insert.bindValue(":previous_maximum_altitude", prevSeg.maxAlt * 100);
    insert.bindValue(":previous_direction", atools::charToStr(prevSeg.dir));
  }

  if(!nextSeg.next.ident.isEmpty())
  {
    insert.bindValue(":next_type", convertType(nextSeg.next.type));
    insert.bindValue(":next_ident", nextSeg.next.ident);
    insert.bindValue(":next_region", nextSeg.next.region);
    insert.bindValue(":next_minimum_altitude", nextSeg.minAlt * 100);
    insert.bindValue(":next_maximum_altitude", nextSeg.maxAlt * 100);
    insert.bindValue(":next_direction", atools::charToStr(nextSeg.dir));
  }

  insert.exec();
}

bool AirwayPostProcess::nextOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2)
{
  if(s1.next.ident == s2.next.ident)
  {
    if(s1.next.region == s2.next.region)
      return s1.next.type < s2.next.type;
    else
      return s1.next.region < s2.next.region;
  }
  else
    return s1.next.ident < s2.next.ident;
}

bool AirwayPostProcess::prevOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2)
{
  if(s1.prev.ident == s2.prev.ident)
  {
    if(s1.prev.region == s2.prev.region)
      return s1.prev.type < s2.prev.type;
    else
      return s1.prev.region < s2.prev.region;
  }
  else
    return s1.prev.ident < s2.prev.ident;
}

bool AirwayPostProcess::findSegment(QVector<AirwaySegment>& found, QSet<AirwaySegment>& done,
                                    const QVector<AirwaySegment>& segments,
                                    AirwayPoint airwayPoint, AirwayPoint excludePoint, bool searchPrevious)
{
  found.clear();

  AirwaySegment segment;
  if(searchPrevious)
    segment.prev = airwayPoint;
  else
    segment.next = airwayPoint;

  QVector<AirwaySegment>::const_iterator lower =
    std::lower_bound(segments.begin(), segments.end(), segment, searchPrevious ? prevOrderFunc : nextOrderFunc);
  QVector<AirwaySegment>::const_iterator upper =
    std::upper_bound(segments.begin(), segments.end(), segment, searchPrevious ? prevOrderFunc : nextOrderFunc);

  // qDebug() << "SEARCH" << idx.ident << idx.region << idx.type << prev;

  for(QVector<AirwaySegment>::const_iterator it = lower; it < upper; ++it)
  {
    // qDebug() << "FOUND" << (*it).prev.ident << (*it).prev.region << (*it).prev.type
    // << (*it).next.ident << (*it).next.region << (*it).next.type;

    if(searchPrevious)
    {
      if(it->next != excludePoint && !done.contains(*it))
      {
        found.append(*it);
        done.insert(*it);
        done.insert((*it).reversed());
      }
    }
    else
    {
      if(it->prev != excludePoint && !done.contains(*it))
      {
        found.append(*it);
        done.insert(*it);
        done.insert((*it).reversed());
      }
    }
  }

  if(found.size() > 1)
    qWarning() << "Found more than one airway segment";

  return !found.isEmpty();
}

} // namespace xp
} // namespace fs
} // namespace atools
