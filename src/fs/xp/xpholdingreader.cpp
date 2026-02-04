/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/xp/xpholdingreader.h"

#include "fs/common/airportindex.h"
#include "fs/common/magdecreader.h"
#include "fs/util/fsutil.h"
#include "fs/xp/xpconstants.h"
#include "geo/calculations.h"
#include "geo/pos.h"

#include "sql/sqlutil.h"
#include "sql/sqlquery.h"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;

namespace atools {
namespace fs {
namespace xp {

enum FieldIndex
{
  IDENT = 0,
  REGION = 1,
  AIRPORT_IDENT = 2,
  TYPE = 3,
  COURSE_MAG = 4,
  LEG_TIME = 5,
  LEG_LENGTH = 6,
  DIR = 7,
  MIN_ALT = 8,
  MAX_ALT = 9,
  SPEED = 10
};

/* Point/navaid type for reference point */
enum HoldFixType
{
  HOLD_WAYPOINT = 11,
  HOLD_NDB = 2,
  HOLD_VOR = 3
};

XpHoldingReader::XpHoldingReader(atools::sql::SqlDatabase& sqlDb,
                                 atools::fs::common::AirportIndex *airportIndexParam,
                                 const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                 atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpReader(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

XpHoldingReader::~XpHoldingReader()
{
  deInitQueries();
}

// holding_id integer primary key,
// file_id integer not null,           -- BGL or dat file of the feature
// airport_id integer,                 -- Reference to airport
// airport_ident varchar(5),           -- ICAO ident
// nav_id integer,                     -- Refers to vor.vor_id or ndb.ndb_id depending on type
// nav_ident varchar(5),               -- ICAO ident
// name varchar(50),
// region varchar(2),                  -- ICAO two letter region identifier
// type varchar(15),                   -- N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end
// mag_var double,                     -- Magnetic variance in degree < 0 for West and > 0 for East
// course double not null,             -- True inbound course
// turn_direction varchar(1) not null, -- L or R
// leg_length double,                  -- Leg distance in NM
// leg_time double,                    -- Leg time in minutes
// minimum_altitude double,            -- Feet or null if not applicable
// maximum_altitude double,            -- Feet or null
// speed integer,                      -- Speed limit in knots or null
// lonx double not null,               -- Reference fix coordinates
// laty double not null,
void XpHoldingReader::read(const QStringList& line, const XpReaderContext& context)
{
  ctx = &context;

  // Avoid duplicates having the exact same values for all fields
  QString key = line.join(QStringLiteral(","));
  if(holdingSet.contains(key))
    return;
  else
    holdingSet.insert(key);

  // . AE701 DA DAAE 11    171.0      1.0      0.0 R     5580    14000      230
  // . AE712 DA DAAE 11     31.0      1.0      0.0 R    10000    14000      210
  // . RASMA DA DAAS 11    107.0      1.0      0.0 R     6730    10000      230
  // . RASMA DA DAAS 11    107.0      0.0      4.0 R     6730    10000      230
  // .    TM DA DAAT  2    219.0      1.0      0.0 R     9450    10000      280
  // . TOPGI DA DABC 11    237.0      0.0      5.0 L     6890    10000      230
  // . TINIF DA DAOF 11    258.0      1.0      0.0 R     4110     8000      230
  // . NIMIR DA DAUG 11    301.0      0.0      4.0 R     4270    10000      220
  QString navIdent = at(line, IDENT);

  QString airportIdent = atAirportIdent(line, AIRPORT_IDENT);
  int airportId = -1;

  // Check for enroute fixes without airport
  if(!airportIdent.isEmpty())
    airportId = airportIndex->getAirportId(airportIdent, false /* allIdents */);
  else
    airportIdent.clear();

  int navId = -1;
  float magvar = 0.f;
  Pos pos;
  QString navType, vorType;
  bool vorDmeOnly = false, vorHasDme = false;

  // Fetch the center fix by ident and region to get id and coordinates
  HoldFixType type = static_cast<HoldFixType>(at(line, TYPE).toInt());
  QString region = at(line, REGION);
  switch(type)
  {
    case HOLD_WAYPOINT:
      fetchWaypoint(navIdent, region, navId, magvar, pos);
      navType = QStringLiteral("W");
      break;

    case HOLD_NDB:
      fetchNdb(navIdent, region, navId, magvar, pos);
      navType = QStringLiteral("N");
      break;

    case HOLD_VOR:
      fetchVor(navIdent, region, navId, magvar, pos, vorType, vorDmeOnly, vorHasDme);
      navType = QStringLiteral("V");
      break;
  }

  insertQuery->bindValue(QStringLiteral(":holding_id"), ++curHoldId);
  insertQuery->bindValue(QStringLiteral(":file_id"), context.curFileId);

  if(airportId != -1)
  {
    insertQuery->bindValue(QStringLiteral(":airport_id"), airportId);
    insertQuery->bindValue(QStringLiteral(":airport_ident"), airportIdent);
  }

  insertQuery->bindValue(QStringLiteral(":nav_id"), navId);
  insertQuery->bindValue(QStringLiteral(":nav_ident"), navIdent);
  insertQuery->bindValue(QStringLiteral(":nav_type"), navType); // N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end

  if(navType == QStringLiteral("V"))
  {
    insertQuery->bindValue(QStringLiteral(":vor_type"), vorType);
    insertQuery->bindValue(QStringLiteral(":vor_dme_only"), vorDmeOnly);
    insertQuery->bindValue(QStringLiteral(":vor_has_dme"), vorHasDme);
  }
  else
  {
    insertQuery->bindNullInt(QStringLiteral(":vor_type"));
    insertQuery->bindNullInt(QStringLiteral(":vor_dme_only"));
    insertQuery->bindNullInt(QStringLiteral(":vor_has_dme"));
  }

  insertQuery->bindValue(QStringLiteral(":region"), region);
  insertQuery->bindValue(QStringLiteral(":mag_var"), magvar);
  insertQuery->bindValue(QStringLiteral(":course"), at(line, COURSE_MAG).toFloat());
  insertQuery->bindValue(QStringLiteral(":turn_direction"), at(line, DIR));
  insertQuery->bindValue(QStringLiteral(":leg_length"), at(line, LEG_LENGTH).toFloat());
  insertQuery->bindValue(QStringLiteral(":leg_time"), at(line, LEG_TIME).toFloat());
  insertQuery->bindValue(QStringLiteral(":minimum_altitude"), at(line, MIN_ALT).toFloat());
  insertQuery->bindValue(QStringLiteral(":maximum_altitude"), at(line, MAX_ALT).toFloat());
  insertQuery->bindValue(QStringLiteral(":speed_limit"), at(line, SPEED).toInt());
  insertQuery->bindValue(QStringLiteral(":lonx"), pos.getLonX());
  insertQuery->bindValue(QStringLiteral(":laty"), pos.getLatY());
  insertQuery->exec();
  insertQuery->clearBoundValues();
}

void XpHoldingReader::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertQuery = new SqlQuery(db);
  insertQuery->prepare(util.buildInsertStatement("holding", QString(), {"name"}));

  initNavQueries();
}

void XpHoldingReader::deInitQueries()
{
  deInitNavQueries();

  delete insertQuery;
  insertQuery = nullptr;
}

void XpHoldingReader::finish(const XpReaderContext& context)
{
  Q_UNUSED(context)
}

void XpHoldingReader::reset()
{
  holdingSet.clear();
}

} // namespace xp
} // namespace fs
} // namespace atools
