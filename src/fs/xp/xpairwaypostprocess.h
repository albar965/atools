/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

struct AirwaySegment;
struct AirwayPoint;

/* X-Plane types */
enum AirwayType
{
  NONE = 0,
  VICTOR = 1,
  JET = 2,
  BOTH = 3 /* artifical type created by merge of victor and jet */
};

/*
 * Takes the unordered from/to and to/from lists from X-Plane and converts them into an ordered list with from/via/to rows.
 * Reads from table airway_temp and inserts into tmp_airway_point
 */
class XpAirwayPostProcess
{
public:
  XpAirwayPostProcess(atools::sql::SqlDatabase& sqlDb);
  virtual ~XpAirwayPostProcess();

  /* Reads all from/to and to/from segments of all airways and creates from/via/to segments. */
  bool postProcessEarthAirway();

private:
  /* Sort and write out all segments of an airway. This also includes multiple fragments of the same airway name.
   * The list segments is emptied during this process. */
  void writeSegments(QList<AirwaySegment>& segments, sql::SqlQuery& insertTmpAirwayPoint, const QString& name, AirwayType type);

  /* Finds an airway segment starting or ending with airwayPoint in the list segments which can sorted by next or prev ids.*/
  bool findSegment(QList<AirwaySegment>& foundSegments, QSet<AirwaySegment>& finshedSegments, const QList<AirwaySegment>& segments,
                   AirwayPoint airwayPoint, AirwayPoint excludePoint, bool searchPrevious);

  /* Write a from/via/to (prev/mid/next) triplet into the database */
  void writeSegment(atools::sql::SqlQuery& insertTmpAirwayPoint, const QString& name, AirwayType type,
                    const AirwaySegment& prevSeg, const AirwaySegment& nextSeg);

  /* Used for sorting and binary search in the ordered segment lists. Sorts by next/to */
  static bool nextOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2);

  /* Used for sorting and binary search in the ordered segment lists. Sorts by previous/from */
  static bool prevOrderFunc(const AirwaySegment& s1, const AirwaySegment& s2);

  atools::sql::SqlDatabase& db;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XP_POSTPROCESS_H
