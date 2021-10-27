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

#include "fs/xp/xpfixwriter.h"

#include "fs/common/airportindex.h"
#include "fs/xp/xpconstants.h"
#include "fs/progresshandler.h"
#include "fs/common/magdecreader.h"
#include "geo/pos.h"

#include "sql/sqlutil.h"
#include "sql/sqlquery.h"

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
  IDENT = 2, /* Usually five characters. Unique within an ICAO region */
  AIRPORT = 3, /* Must be either airport identifier or "ENRT" */
  REGION = 4,
  ARINC_TYPE = 5 /* 32 bit representation of the 3-byte field defined by ARINC
                  * 424.18 field type definition 5.42, with the 4th byte set to 0 in
                  * Little Endian byte order. This field can be empty ONLY for user
                  * waypoints in user_fix.dat */
};

XpFixWriter::XpFixWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                         const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                         atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

XpFixWriter::~XpFixWriter()
{
  deInitQueries();
}

void XpFixWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  atools::geo::Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  insertWaypointQuery->bindValue(":waypoint_id", ++curFixId);
  insertWaypointQuery->bindValue(":file_id", context.curFileId);
  insertWaypointQuery->bindValue(":ident", at(line, IDENT));
  insertWaypointQuery->bindValue(":airport_id", airportIndex->getAirportIdVar(at(line, AIRPORT)));
  insertWaypointQuery->bindValue(":airport_ident", atAirportIdent(line, AIRPORT));

  insertWaypointQuery->bindValue(":region", at(line, REGION)); // ZZ for no region

  insertWaypointQuery->bindValue(":type", "WN"); // All named waypoints

  // 32bit representation of the 3-byte field defined by ARINC
  // 424.18 field type definition 5.42, with the 4th byte set to 0 in
  // Little Endian byte order. This field can be empty ONLY for user
  // waypoints in user_fix.dat
  union
  {
    quint32 intValue;
    unsigned char byteValue[4];
  } u;
  u.intValue = at(line, ARINC_TYPE).toUInt();
  QString arincTypeStr;
  arincTypeStr.append(QChar(u.byteValue[0])).append(QChar(u.byteValue[1])).
  append(QChar(u.byteValue[2])).append(QChar(u.byteValue[3]));
  insertWaypointQuery->bindValue(":arinc_type", arincTypeStr);

  insertWaypointQuery->bindValue(":num_victor_airway", 0); // filled  by sql/fs/db/xplane/prepare_airway.sql
  insertWaypointQuery->bindValue(":num_jet_airway", 0); // as above
  insertWaypointQuery->bindValue(":mag_var", context.magDecReader->getMagVar(pos));
  insertWaypointQuery->bindValue(":lonx", pos.getLonX());
  insertWaypointQuery->bindValue(":laty", pos.getLatY());
  insertWaypointQuery->exec();

  progress->incNumWaypoints();
}

void XpFixWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context)
}

void XpFixWriter::reset()
{

}

void XpFixWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertWaypointQuery = new SqlQuery(db);
  insertWaypointQuery->prepare(util.buildInsertStatement("waypoint", QString(), {"nav_id"}));
}

void XpFixWriter::deInitQueries()
{
  delete insertWaypointQuery;
  insertWaypointQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
