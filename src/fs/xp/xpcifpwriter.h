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

#ifndef ATOOLS_FS_XP_CIFPWRITER_H
#define ATOOLS_FS_XP_CIFPWRITER_H

#include "fs/xp/xpwriter.h"

#include "sql/sqlrecord.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;

namespace xp {

/*
 * Reads earth_fix.dat and writes to waypoint table.
 */
class XpAirportIndex;

namespace rc {
enum RowCode
{
  NONE,
  APPROACH,
  SID,
  STAR,
  RWY,
  PRDAT
};

}

/*
 * Reads a CIFP file and writes all approaches,transitons, SIDs and STARs into the database.
 */
class XpCifpWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpCifpWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::xp::XpAirportIndex *xpAirportIndex,
               const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler);
  virtual ~XpCifpWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;

private:
  /* Used to store a procedure before writing to the database */
  struct Procedure
  {
    Procedure()
    {
    }

    Procedure(rc::RowCode rc, const atools::sql::SqlRecord& rec)
      : rowCode(rc), record(rec)
    {
    }

    QStringList runways;
    rc::RowCode rowCode = rc::NONE;
    atools::sql::SqlRecord record;
    atools::sql::SqlRecordVector legRecords;
  };

  void initQueries();
  void deInitQueries();

  /* Write an approach, SID, STAR or transition */
  void writeProcedure(const QStringList& line, const XpWriterContext& context);

  /* Write an approach, SID, STAR or transition leg */
  void writeProcedureLeg(const QStringList& line, const XpWriterContext& context);

  /* Fill a leg for the transition_leg or approach_leg table */
  void bindLeg(const QStringList& line, sql::SqlRecord& rec, const XpWriterContext& context);

  /* Write an approach, SID, STAR */
  void writeApproach(const QStringList& line, const XpWriterContext& context);
  void writeApproachLeg(const QStringList& line, const XpWriterContext& context);

  /* Write a transition */
  void writeTransition(const QStringList& line, const XpWriterContext& context);
  void writeTransitionLeg(const QStringList& line, const XpWriterContext& context);

  /* Reorder and duplicate procedures and legs, then write into the database */
  void finishProcedure(const XpWriterContext& context);

  atools::fs::xp::rc::RowCode toRowCode(const QString& code, const XpWriterContext& context);

  /* Calculate a navaid type based on section and subsection code */
  QString navaidType(const QString& sectionCode, const QString& subSectionCode, const XpWriterContext& context);

  /* Calculate a database procedure type based on route type */
  QString procedureType(char routeType, const XpWriterContext& context);

  /* Assigns new ids to the currently stored approaches */
  void assignApproachIds(XpCifpWriter::Procedure& proc);

  /* Assigns new ids to the currently stored transitions */
  void assignTransitionIds(XpCifpWriter::Procedure& proc);

  /* Extract runway names */
  QString apprRunwayNameAndSuffix(const QString& ident, QString& suffix, const XpWriterContext& context);
  QString sidStarRunwayNameAndSuffix(const QString& ident, const XpWriterContext& context);

  int curApproachId = 0, curTransitionId = 0, curApproachLegId = 0, curTransitionLegId = 0;

  int numProcedures = 0;

  atools::sql::SqlQuery *insertApproachQuery = nullptr, *insertTransitionQuery = nullptr,
                        *insertApproachLegQuery = nullptr, *insertTransitionLegQuery = nullptr,
                        *updateAirportQuery = nullptr;

  atools::fs::xp::XpAirportIndex *airportIndex;
  const atools::sql::SqlRecord approachRecord, approachLegRecord, transitionRecord, transitionLegRecord;

  QVector<Procedure> approaches;
  QVector<Procedure> transitions;

  rc::RowCode curRowCode;
  int curSeqNo = std::numeric_limits<int>::max();
  char curRouteType = ' ';
  QString curRouteIdent, curTransIdent;

  bool writingMissedApproach = false;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_CIFPWRITER_H
