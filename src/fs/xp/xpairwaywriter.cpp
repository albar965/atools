/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "fs/xp/xpairwaywriter.h"

#include "sql/sqlutil.h"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

// ABCDE K1 11 ABC   K1  3 N 2 180 450 J13
// ABC   K1 3  DEF   K2  3 N 2 180 450 J13
// DEF   K2 3  KLMNO K2 11 F 2 180 450 J13-J14-J15
enum FieldIndex
{
  FROM_IDENT = 0,
  FROM_REGION = 1,
  FROM_TYPE = 2,
  TO_IDENT = 3,
  TO_REGION = 4,
  TO_TYPE = 5,
  DIRECTION = 6,
  TYPE = 7,
  MIN_ALT = 8,
  MAX_ALT = 9,
  NAME = 10
};

XpAirwayWriter::XpAirwayWriter(atools::sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                               ProgressHandler *progressHandler, atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors)
{
  initQueries();
}

atools::fs::xp::XpAirwayWriter::~XpAirwayWriter()
{
  deInitQueries();
}

void XpAirwayWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  QString names = at(line, NAME);

  for(const QString& name : names.split("-"))
  {
    // Split dash separated airway list
    insertAirwayQuery->bindValue(":airway_temp_id", ++curAirwayId);
    insertAirwayQuery->bindValue(":name", name);
    insertAirwayQuery->bindValue(":type", at(line, TYPE).toInt());
    insertAirwayQuery->bindValue(":direction", at(line, DIRECTION));
    insertAirwayQuery->bindValue(":minimum_altitude", at(line, MIN_ALT).toInt());
    insertAirwayQuery->bindValue(":maximum_altitude", at(line, MAX_ALT).toInt());

    insertAirwayQuery->bindValue(":previous_ident", at(line, FROM_IDENT));
    insertAirwayQuery->bindValue(":previous_region", at(line, FROM_REGION));
    insertAirwayQuery->bindValue(":previous_type", at(line, FROM_TYPE).toInt());

    insertAirwayQuery->bindValue(":next_ident", at(line, TO_IDENT));
    insertAirwayQuery->bindValue(":next_region", at(line, TO_REGION));
    insertAirwayQuery->bindValue(":next_type", at(line, TO_TYPE).toInt());

    insertAirwayQuery->exec();
  }
}

void XpAirwayWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context);
}

void XpAirwayWriter::reset()
{

}

void XpAirwayWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertAirwayQuery = new SqlQuery(db);
  insertAirwayQuery->prepare(util.buildInsertStatement("airway_temp"));
}

void XpAirwayWriter::deInitQueries()
{
  delete insertAirwayQuery;
  insertAirwayQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
