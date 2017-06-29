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

#include "fs/xp/airwaywriter.h"

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

AirwayWriter::AirwayWriter(atools::sql::SqlDatabase& sqlDb)
  : Writer(sqlDb)
{
  initQueries();
}

atools::fs::xp::AirwayWriter::~AirwayWriter()
{
  deInitQueries();
}

void AirwayWriter::write(const QStringList& line, int curFileId)
{
  Q_UNUSED(curFileId);

  QString names = line.at(NAME);

  for(const QString& name : names.split("-"))
  {
    insertAirwayQuery->bindValue(":airway_temp_id", ++curAirwayId);
    insertAirwayQuery->bindValue(":name", name);
    insertAirwayQuery->bindValue(":type", line.at(TYPE).toInt());
    insertAirwayQuery->bindValue(":direction", line.at(DIRECTION));
    insertAirwayQuery->bindValue(":minimum_altitude", line.at(MIN_ALT).toInt());
    insertAirwayQuery->bindValue(":maximum_altitude", line.at(MAX_ALT).toInt());

    insertAirwayQuery->bindValue(":previous_ident", line.at(FROM_IDENT));
    insertAirwayQuery->bindValue(":previous_region", line.at(FROM_REGION));
    insertAirwayQuery->bindValue(":previous_type", line.at(FROM_TYPE).toInt());

    insertAirwayQuery->bindValue(":next_ident", line.at(TO_IDENT));
    insertAirwayQuery->bindValue(":next_region", line.at(TO_REGION));
    insertAirwayQuery->bindValue(":next_type", line.at(TO_TYPE).toInt());

    insertAirwayQuery->exec();
  }
}

void AirwayWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertAirwayQuery = new SqlQuery(db);
  insertAirwayQuery->prepare(util.buildInsertStatement("airway_temp"));
}

void AirwayWriter::deInitQueries()
{
  delete insertAirwayQuery;
  insertAirwayQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
