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

#include "fs/xp/xpairwaypostprocess.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "atools.h"

#include <QDebug>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

// ==================================================================
/* X-Plane types */
enum AirwayPointType
{
  AW_NONE = 0,
  AW_FIX = 11,
  AW_NDB = 2,
  AW_VOR = 3
};

/* Waypoint along the airway */
struct AirwayPoint
{
  QString ident, region;
  AirwayPointType type = AW_NONE;
};

inline bool operator==(const AirwayPoint& seg1, const AirwayPoint& seg2)
{
  return seg1.ident == seg2.ident && seg1.region == seg2.region && seg1.type == seg2.type;
}

inline bool operator!=(const AirwayPoint& seg1, const AirwayPoint& seg2)
{
  return !(seg1 == seg2);
}

inline size_t qHash(const AirwayPoint& seg)
{
  return qHash(seg.ident) ^ qHash(seg.region) ^ static_cast<uint>(seg.type);
}

QDebug operator<<(QDebug out, const AirwayPoint& point)
{
  out << point.ident << "/" << point.region << point.type;
  return out;
}

// ==================================================================
enum SegmentDir : char {DIR_NONE = '\0', DIR_BACKWARD = 'B', DIR_FORWARD = 'F'};

/* from/to airway segment */
struct AirwaySegment
{
  AirwayPoint prev, next;
  int minAlt = 0, maxAlt = 0;
  SegmentDir dir = DIR_NONE;

  /* reverse prev/next, direction and return a copy */
  AirwaySegment reversed() const
  {
    AirwaySegment segment(*this);
    segment.prev.ident.swap(segment.next.ident);
    segment.prev.region.swap(segment.next.region);
    std::swap(segment.prev.type, segment.next.type);

    if(segment.dir == DIR_BACKWARD)
      segment.dir = DIR_FORWARD;
    else if(dir == DIR_FORWARD)
      segment.dir = DIR_BACKWARD;

    return segment;
  }

};

inline bool operator==(const AirwaySegment& seg1, const AirwaySegment& seg2)
{
  return seg1.next == seg2.next && seg1.prev == seg2.prev;
}

inline bool operator!=(const AirwaySegment& seg1, const AirwaySegment& seg2)
{
  return !(seg1 == seg2);
}

inline size_t qHash(const AirwaySegment& seg)
{
  return qHash(seg.prev) ^ qHash(seg.next);
}

QDebug operator<<(QDebug out, const AirwaySegment& segment)
{
  out << "prev" << segment.prev << "next" << segment.next << QChar(static_cast<char>(segment.dir));
  return out;
}

QDebug operator<<(QDebug out, const QList<AirwaySegment>& segments)
{
  out << Qt::endl;
  for(const AirwaySegment& seg: segments)
    out << seg << Qt::endl;
  return out;
}

/* Convert X-Plane type into database type */
QString convertType(AirwayPointType xptype)
{
  switch(xptype)
  {
    case atools::fs::xp::AW_NONE:
      return QString();

    case atools::fs::xp::AW_FIX:
      return "O";

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
XpAirwayPostProcess::XpAirwayPostProcess(sql::SqlDatabase& sqlDb)
  : db(sqlDb)
{
}

XpAirwayPostProcess::~XpAirwayPostProcess()
{
}

bool XpAirwayPostProcess::postProcessEarthAirway()
{
  SqlQuery airwayTempQuery("select name, type, direction, minimum_altitude, maximum_altitude, "
                           "previous_type, previous_ident, previous_region, "
                           "next_type, next_ident, next_region from tmp_airway order by name", db); // where name = 'Y655'

  SqlQuery insertTmpAirwayPoint(db);
  insertTmpAirwayPoint.prepare(SqlUtil(db).buildInsertStatement("tmp_airway_point", QString(),
                                                                {"airway_point_id", "next_airport_ident", "previous_airport_ident"}));

  QString currentAirway;
  AirwayType currentAirwayType = NONE;
  QList<AirwaySegment> segments;

  // Read duplets from temp table
  while(airwayTempQuery.next())
  {
    QString airway = airwayTempQuery.valueStr("name");
    AirwayType airwayType = static_cast<AirwayType>(airwayTempQuery.valueInt("type"));

    if(currentAirway.isEmpty())
    {
      // First iteration
      currentAirway = airway;
      currentAirwayType = airwayType;
    }

    // Test if airway has changed ======================================
    if(currentAirway != airway && !segments.isEmpty())
    {
      // Airway name or type has changed - order and write all its segments
      writeSegments(segments, insertTmpAirwayPoint, currentAirway, currentAirwayType);

      segments.clear();
      currentAirway = airway;
      currentAirwayType = airwayType;
    }

    // Add a segment from the database
    AirwaySegment segment;
    segment.minAlt = airwayTempQuery.valueInt("minimum_altitude");
    segment.maxAlt = airwayTempQuery.valueInt("maximum_altitude");
    segment.next.ident = airwayTempQuery.valueStr("next_ident");
    segment.next.region = airwayTempQuery.valueStr("next_region");
    segment.next.type = static_cast<AirwayPointType>(airwayTempQuery.valueInt("next_type"));
    segment.prev.ident = airwayTempQuery.valueStr("previous_ident");
    segment.prev.region = airwayTempQuery.valueStr("previous_region");
    segment.prev.type = static_cast<AirwayPointType>(airwayTempQuery.valueInt("previous_type"));
    segment.dir = static_cast<SegmentDir>(atools::strToChar(airwayTempQuery.valueStr("direction")));

    segments.insert(segments.size(), segment);
  }

  // Write the last airway
  if(!segments.isEmpty())
    writeSegments(segments, insertTmpAirwayPoint, currentAirway, currentAirwayType);

  return false;
}

void XpAirwayPostProcess::writeSegments(QList<AirwaySegment>& segments, SqlQuery& insertTmpAirwayPoint, const QString& name,
                                        AirwayType type)
{
  // W66: NUKTI/ZW > GOVSA/ZL > JNQ/ZL > GOBIN/ZL > ATBUG/ZL > DKO/ZB
  // if(name != "W66")
  // return;

  // Create a list ordered by next waypoint id
  QList<AirwaySegment> segsByNext(segments);
  std::sort(segsByNext.begin(), segsByNext.end(), nextOrderFunc);

  // Create a list ordered by previous waypoint id
  QList<AirwaySegment> segsByPrev(segments);
  std::sort(segsByPrev.begin(), segsByPrev.end(), prevOrderFunc);

  // List of finished segments with original and reversed from/to
  QSet<AirwaySegment> finishedSegments;

#ifdef DEBUG_INFORMATION_LOAD_XP_AIRWAYS
  qDebug() << Q_FUNC_INFO << "BYNEXT" << segsByNext;
  qDebug() << Q_FUNC_INFO << "BYPREV" << segsByPrev;
#endif

  // Iterate over all segments of this airway
  while(!segments.isEmpty())
  {
    // Create an airway fragment
    QList<AirwaySegment> sortedSegments;

    // Insert start segment
    sortedSegments.append(segments.constFirst());

    // Start segment is finished here - add from/to and to/from to simplify search
    finishedSegments.insert(segments.constFirst());
    finishedSegments.insert(segments.constFirst().reversed());
    segments.removeFirst();

    bool foundPrev = true, foundNext = true;
    while(foundNext || foundPrev)
    {
      // Find next for last segment
      QList<AirwaySegment> foundSegments;
      const AirwaySegment& last = sortedSegments.constLast();
      if(findSegment(foundSegments, finishedSegments, segsByPrev, last.next, last.prev, true))
      {
        // Found segment is in correct order
        segments.removeOne(foundSegments.constFirst());
        sortedSegments.append(foundSegments.constFirst());
        foundNext = true;
      }
      else if(findSegment(foundSegments, finishedSegments, segsByNext, last.next, last.prev, false))
      {
        // Found segment is in reversed order
        segments.removeOne(foundSegments.constFirst());
        sortedSegments.append(foundSegments.constFirst().reversed());
        foundNext = true;
      }
      else
        foundNext = false;

      // Find previous for first segment
      const AirwaySegment& first = sortedSegments.constFirst();
      if(findSegment(foundSegments, finishedSegments, segsByNext, first.prev, first.next, false))
      {
        // Found segment is in correct order
        segments.removeOne(foundSegments.constFirst());
        sortedSegments.prepend(foundSegments.constFirst());
        foundPrev = true;
      }
      else if(findSegment(foundSegments, finishedSegments, segsByPrev, first.prev, first.next, true))
      {
        // Found segment is in reversed order
        segments.removeOne(foundSegments.constFirst());
        sortedSegments.prepend(foundSegments.constFirst().reversed());
        foundPrev = true;
      }
      else
        foundPrev = false;
    }

#ifdef DEBUG_INFORMATION_LOAD_XP_AIRWAYS
    qDebug() << Q_FUNC_INFO << "-------";
    qDebug() << Q_FUNC_INFO << "SORTED" << sortedSegments;
    qDebug() << Q_FUNC_INFO << "-------";
#endif

    // Write the from/via/to triplets now
    for(int i = 0; i < sortedSegments.size(); i++)
    {
      // use value method to get default constructed objects if index is invalid
      AirwaySegment prev12 = sortedSegments.value(i - 1); // 1 -> 2
      AirwaySegment mid23 = sortedSegments.value(i); // 2 -> 3
      AirwaySegment next34 = sortedSegments.value(i + 1); // 3 -> 4

      // Write in order 1 -> 2 -> 3
#ifdef DEBUG_INFORMATION_LOAD_XP_AIRWAYS
      qDebug() << Q_FUNC_INFO << "1 -> 2 -> 3 ###################################################";
      qDebug() << Q_FUNC_INFO << name << prev12 << mid23;
#endif

      // Write in order 2 -> 3 -> 4
#ifdef DEBUG_INFORMATION_LOAD_XP_AIRWAYS
      qDebug() << Q_FUNC_INFO << "2 -> 3 -> 4 ######";
      qDebug() << Q_FUNC_INFO << name << mid23 << next34;
#endif

      writeSegment(insertTmpAirwayPoint, name, type, mid23, next34);
    }
  }
}

void XpAirwayPostProcess::writeSegment(SqlQuery& insertTmpAirwayPoint, const QString& name, AirwayType type,
                                       const AirwaySegment& prevSeg, const AirwaySegment& nextSeg)
{
  insertTmpAirwayPoint.clearBoundValues();

  if(prevSeg.next.ident.isEmpty())
    qWarning() << Q_FUNC_INFO << "Airway" << name << "Empty mid ident";

  if(nextSeg.next.ident.isEmpty() && prevSeg.prev.ident.isEmpty())
    qWarning() << Q_FUNC_INFO << "Airway" << name << "Empty prev and next ident";

  insertTmpAirwayPoint.bindValue(":name", name);
  insertTmpAirwayPoint.bindValue(":type", convertAirwayType(type));
  insertTmpAirwayPoint.bindValue(":mid_type", convertType(prevSeg.next.type));
  insertTmpAirwayPoint.bindValue(":mid_ident", prevSeg.next.ident);
  insertTmpAirwayPoint.bindValue(":mid_region", prevSeg.next.region);

  if(!prevSeg.prev.ident.isEmpty())
  {
    insertTmpAirwayPoint.bindValue(":previous_type", convertType(prevSeg.prev.type));
    insertTmpAirwayPoint.bindValue(":previous_ident", prevSeg.prev.ident);
    insertTmpAirwayPoint.bindValue(":previous_region", prevSeg.prev.region);
    insertTmpAirwayPoint.bindValue(":previous_minimum_altitude", prevSeg.minAlt * 100);
    insertTmpAirwayPoint.bindValue(":previous_maximum_altitude", prevSeg.maxAlt * 100);
    insertTmpAirwayPoint.bindValue(":previous_direction", atools::charToStr(prevSeg.dir));
  }

  if(!nextSeg.next.ident.isEmpty())
  {
    insertTmpAirwayPoint.bindValue(":next_type", convertType(nextSeg.next.type));
    insertTmpAirwayPoint.bindValue(":next_ident", nextSeg.next.ident);
    insertTmpAirwayPoint.bindValue(":next_region", nextSeg.next.region);
    insertTmpAirwayPoint.bindValue(":next_minimum_altitude", nextSeg.minAlt * 100);
    insertTmpAirwayPoint.bindValue(":next_maximum_altitude", nextSeg.maxAlt * 100);
    insertTmpAirwayPoint.bindValue(":next_direction", atools::charToStr(nextSeg.dir));
  }

  insertTmpAirwayPoint.exec();
}

bool XpAirwayPostProcess::nextOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2)
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

bool XpAirwayPostProcess::prevOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2)
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

bool XpAirwayPostProcess::findSegment(QList<AirwaySegment>& foundSegments, QSet<AirwaySegment>& finshedSegments,
                                      const QList<AirwaySegment>& segments, AirwayPoint airwayPoint, AirwayPoint excludePoint,
                                      bool searchPrevious)
{
  foundSegments.clear();

  AirwaySegment segment;
  if(searchPrevious)
    segment.prev = airwayPoint;
  else
    segment.next = airwayPoint;

  auto lower = std::lower_bound(segments.constBegin(), segments.constEnd(), segment, searchPrevious ? prevOrderFunc : nextOrderFunc);
  auto upper = std::upper_bound(segments.constBegin(), segments.constEnd(), segment, searchPrevious ? prevOrderFunc : nextOrderFunc);

  for(auto it = lower; it < upper; ++it)
  {
    if(searchPrevious)
    {
      if(it->next != excludePoint && !finshedSegments.contains(*it))
      {
        foundSegments.append(*it);
        finshedSegments.insert(*it);
        finshedSegments.insert((*it).reversed());
      }
    }
    else
    {
      if(it->prev != excludePoint && !finshedSegments.contains(*it))
      {
        foundSegments.append(*it);
        finshedSegments.insert(*it);
        finshedSegments.insert((*it).reversed());
      }
    }
  }

  if(foundSegments.size() > 1)
    qWarning() << "Found more than one airway segment";

  return !foundSegments.isEmpty();
}

} // namespace xp
} // namespace fs
} // namespace atools
