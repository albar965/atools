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

#ifndef ATOOLS_XP_POSTPROCESS_H
#define ATOOLS_XP_POSTPROCESS_H

#include <QString>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
class NavDatabaseOptions;
class ProgressHandler;

namespace xp {

/* X-Plane types */
enum AirwayPointType
{
  AW_FIX = 11,
  AW_NDB = 2,
  AW_VOR = 3
};

/* X-Plane types */
enum AirwayType
{
  NONE = 0,
  VICTOR = 1,
  JET = 2,
  BOTH = 3 /* artifical type created by merge of victor and jet */
};

/* Waypoint along the airway */
struct AirwayPoint
{
  QString ident, region;
  AirwayPointType type;
};

/* from/to airway segment */
struct AirwaySegment
{
  AirwayPoint prev, next;
  int minAlt;

  /* reverse from/to */
  AirwaySegment reverse() const
  {
    AirwaySegment retval(*this);
    retval.prev.ident.swap(retval.next.ident);
    retval.prev.region.swap(retval.next.region);
    std::swap(retval.prev.type, retval.next.type);
    return retval;
  }

};

/*
 * Takes the unordered from/to and to/from lists from X-Plane and converts them into an ordered list with from/via/to rows.
 * Reads from table airway_temp and inserts into airway_point
 */
class AirwayPostProcess
{
public:
  AirwayPostProcess(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                    atools::fs::ProgressHandler *progress);
  virtual ~AirwayPostProcess();

  bool postProcessEarthAirway();

private:
  void writeSegments(QList<AirwaySegment>& segments, sql::SqlQuery& insert, const QString& name, AirwayType type);

  bool findSegment(QVector<AirwaySegment>& found, QSet<AirwaySegment>& done, const QVector<AirwaySegment>& segments,
                   AirwayPoint airwayPoint, AirwayPoint excludePoint, bool searchPrevious);

  void writeSegment(const AirwayPoint& prev, const AirwayPoint& mid, const AirwayPoint& next,
                    atools::sql::SqlQuery& insert, const QString& name, AirwayType type,
                    int prevMinAlt, int nextMinAlt);

  static bool nextOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2);
  static bool prevOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2);

  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progressHandler = nullptr;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_POSTPROCESS_H
