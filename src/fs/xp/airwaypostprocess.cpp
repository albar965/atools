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

#include "fs/xp/airwaypostprocess.h"

#include "fs/navdatabaseoptions.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "exception.h"

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

/* Reads all from/to and to/from segments of all airways and creates from/via/to segments. */
bool AirwayPostProcess::postProcessEarthAirway()
{
  // A29
  // A123 - 1
  // A105 - 2
  // A126 -5
  // A125 -8
  // V16 -216
  // where name in ('V16')
  SqlQuery query("select name, type, minimum_altitude, "
                 "previous_type, previous_ident, previous_region, "
                 "next_type, next_ident, next_region from airway_temp order by name", db);

  SqlQuery insert(db);
  insert.prepare(SqlUtil(db).buildInsertStatement("airway_point", QString(),
                                                  {"airway_point_id", "waypoint_id", "next_airport_ident",
                                                   "previous_airport_ident"}));

  QString currentAirway;
  AirwayType currentAirwayType = NONE;
  QList<AirwaySegment> segments;

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
      // Airway has  changed order and write all its segments
      writeSegments(segments, insert, currentAirway, currentAirwayType);

      segments.clear();
      currentAirway = airway;
      currentAirwayType = airwayType;
    }

    // Add a segment from the database
    AirwaySegment segment;
    segment.minAlt = query.value("minimum_altitude").toInt();
    segment.next.ident = query.value("next_ident").toString();
    segment.next.region = query.value("next_region").toString();
    segment.next.type = static_cast<AirwayPointType>(query.value("next_type").toInt());
    segment.prev.ident = query.value("previous_ident").toString();
    segment.prev.region = query.value("previous_region").toString();
    segment.prev.type = static_cast<AirwayPointType>(query.value("previous_type").toInt());

    segments.insert(segments.size(), segment);
  }

  // Write the last airway
  if(!segments.isEmpty())
    writeSegments(segments, insert, currentAirway, currentAirwayType);

  return false;
}

/* Sort and write out all segments of an airway. This also includes multiple fragments of the same airway name.
 * The list segments is emptied during this process. */
void AirwayPostProcess::writeSegments(QList<AirwaySegment>& segments, SqlQuery& insert, const QString& name,
                                      AirwayType type)
{
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

  while(!segments.isEmpty())
  {
    // Create an airway fragment
    QVector<AirwaySegment> found;
    QVector<AirwaySegment> sortedSegments;

    // Insert start segment
    sortedSegments.append(segments.first());

    // Start segment is finished here - add from/to and to/from to simplify search
    done.insert(segments.first());
    done.insert(segments.first().reverse());
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
        sortedSegments.append(found.first().reverse());
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
        sortedSegments.prepend(found.first().reverse());
        foundPrev = true;
      }
      else
        foundPrev = false;
    }

    //// SAVE
    // qDebug() << "-------";
    // for(const Segment& s: sortedSegments)
    // qDebug() << "SORTED" << s.prev.ident << s.prev.region << s.prev.type
    // << s.next.ident << s.next.region << s.next.type;
    // qDebug() << "-------";
    //// return;

    // Write the from/via/to triplets now
    for(int i = 0; i < sortedSegments.size(); i++)
    {
      // use value to get default constructed objects if index is invalid
      const AirwaySegment& prev = sortedSegments.value(i - 1);
      const AirwaySegment& mid = sortedSegments.value(i);
      const AirwaySegment& next = sortedSegments.value(i + 1);

      writeSegment(prev.prev, mid.prev, mid.next, insert, name, type, prev.minAlt, mid.minAlt);
      writeSegment(mid.prev, mid.next, next.next, insert, name, type, mid.minAlt, next.minAlt);
    }
  }
}

/* Write a from/via/to (prev/mid/next) triplet into the database */
void AirwayPostProcess::writeSegment(const AirwayPoint& prev, const AirwayPoint& mid, const AirwayPoint& next,
                                     SqlQuery& insert, const QString& name, AirwayType type,
                                     int prevMinAlt, int nextMinAlt)
{
  insert.bindValue(":name", name);
  insert.bindValue(":type", convertAirwayType(type));
  insert.bindValue(":mid_type", convertType(mid.type));
  insert.bindValue(":mid_ident", mid.ident);
  insert.bindValue(":mid_region", mid.region);

  if(!prev.ident.isEmpty())
  {
    insert.bindValue(":previous_type", prev.type);
    insert.bindValue(":previous_ident", prev.ident);
    insert.bindValue(":previous_region", prev.region);
    insert.bindValue(":previous_minimum_altitude", prevMinAlt * 100);
  }
  else
  {
    insert.bindValue(":previous_type", QVariant(QVariant::Int));
    insert.bindValue(":previous_ident", QVariant(QVariant::String));
    insert.bindValue(":previous_region", QVariant(QVariant::String));
    insert.bindValue(":previous_minimum_altitude", QVariant(QVariant::Int));
  }
  if(!next.ident.isEmpty())
  {
    insert.bindValue(":next_type", next.type);
    insert.bindValue(":next_ident", next.ident);
    insert.bindValue(":next_region", next.region);
    insert.bindValue(":next_minimum_altitude", nextMinAlt * 100);
  }
  else
  {
    insert.bindValue(":next_type", QVariant(QVariant::Int));
    insert.bindValue(":next_ident", QVariant(QVariant::String));
    insert.bindValue(":next_region", QVariant(QVariant::String));
    insert.bindValue(":next_minimum_altitude", QVariant(QVariant::Int));
  }
  insert.exec();
}

/* Used for sorting and binary search in the ordered segment lists. Sorts by next/to */
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

/* Used for sorting and binary search in the ordered segment lists. Sorts by previous/from */
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

/* Finds an airway segment starting or ending with airwayPoint in the list segments which can sorted by next or prev ids.*/
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
        done.insert((*it).reverse());
      }
    }
    else
    {
      if(it->prev != excludePoint && !done.contains(*it))
      {
        found.append(*it);
        done.insert(*it);
        done.insert((*it).reverse());
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
