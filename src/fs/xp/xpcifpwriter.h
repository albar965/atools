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
  void initQueries();
  void deInitQueries();

  void writeProcedure(const QStringList& line, const XpWriterContext& context);
  void writeProcedureLeg(const QStringList& line, const XpWriterContext& context);
  void bindLeg(const QStringList& line, sql::SqlRecord& rec, const XpWriterContext& context);

  void writeApproach(const QStringList& line, const XpWriterContext& context);
  void writeApproachLeg(const QStringList& line, const XpWriterContext& context);

  void writeTransition(const QStringList& line, const XpWriterContext& context);
  void writeTransitionLeg(const QStringList& line, const XpWriterContext& context);
  void finishProcedure();

  void writeStarApproach(const QStringList& line, const XpWriterContext& context);
  void writeStarApproachLeg(const QStringList& line, const XpWriterContext& context);

  void writeStarTransition(const QStringList& line, const XpWriterContext& context);
  void writeStarTransitionLeg(const QStringList& line, const XpWriterContext& context);

  void bindSidApproach(const QStringList& line, const XpWriterContext& context);
  void writeSidApproachLeg(const QStringList& line, const XpWriterContext& context);

  void bindSidTransition(const QStringList& line, const XpWriterContext& context);
  void writeSidTransitionLeg(const QStringList& line, const XpWriterContext& context);

  atools::fs::xp::rc::RowCode toRowCode(const QString& code, const XpWriterContext& context);
  QString navaidType(const QString& sectionCode, const QString& subSectionCode, const XpWriterContext& context);
  QString procedureType(char routeType, const XpWriterContext& context);

  int curApproachId = 1, curTransitionId = 0, curApproachLegId = 0, curTransitionLegId = 0;

  int numProcedures = 0;

  atools::sql::SqlQuery *insertApproachQuery = nullptr, *insertTransitionQuery = nullptr,
                        *insertApproachLegQuery = nullptr, *insertTransitionLegQuery = nullptr,
                        *updateAirportQuery = nullptr;

  atools::fs::xp::XpAirportIndex *airportIndex;
  const atools::sql::SqlRecord approachRecord, approachLegRecord, transitionRecord, transitionLegRecord;

  atools::sql::SqlRecordVector approachRecords, transitionRecords;
  QList<atools::sql::SqlRecordVector> approachLegRecords, transitionLegRecords;

  rc::RowCode curRowCode;
  int curSeqNo = std::numeric_limits<int>::max();
  char curRouteType = ' ';
  QString curRouteIdent, curTransIdent;

  bool writingApproach = false, writingMissedApproach = false, writingSid = false, writingStar = false;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_CIFPWRITER_H
