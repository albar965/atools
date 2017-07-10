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

#include "fs/xp/xpfixwriter.h"

#include "sql/sqlutil.h"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

// lat lon
// ("28.000708333", "-83.423330556", "KNOST", "ENRT", "K7")
enum FieldIndex
{
  LATY = 0,
  LONX = 1,
  IDENT = 2,
  AIRPORT = 3,
  REGION = 4
};

XpFixWriter::XpFixWriter(atools::sql::SqlDatabase& sqlDb)
  : XpWriter(sqlDb)
{
  initQueries();
}

XpFixWriter::~XpFixWriter()
{
  deInitQueries();
}

void XpFixWriter::write(const QStringList& line, int curFileId)
{
  insertWaypointQuery->bindValue(":waypoint_id", ++curFixId);
  insertWaypointQuery->bindValue(":file_id", curFileId);
  insertWaypointQuery->bindValue(":ident", line.at(IDENT));
  // TODO insertFixQuery->bindValue(":airport_ident", fields.at(3)); or ENRT for enroute
  insertWaypointQuery->bindValue(":region", line.at(REGION)); // ZZ for no region
  insertWaypointQuery->bindValue(":type", "WN");
  insertWaypointQuery->bindValue(":num_victor_airway", 0);
  insertWaypointQuery->bindValue(":num_jet_airway", 0);
  insertWaypointQuery->bindValue(":mag_var", 0);
  insertWaypointQuery->bindValue(":lonx", line.at(LONX).toFloat());
  insertWaypointQuery->bindValue(":laty", line.at(LATY).toFloat());
  insertWaypointQuery->exec();
}

void XpFixWriter::finish()
{

}

void XpFixWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertWaypointQuery = new SqlQuery(db);
  insertWaypointQuery->prepare(util.buildInsertStatement("waypoint", QString(), {"airport_id", "nav_id"}));
}

void XpFixWriter::deInitQueries()
{
  delete insertWaypointQuery;
  insertWaypointQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
