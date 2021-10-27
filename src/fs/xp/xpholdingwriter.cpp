/*****************************************************************************
* Copyright 2015-2021 Alexander Barthel alex@littlenavmap.org
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

#include "fs/xp/xpholdingwriter.h"

#include "fs/common/airportindex.h"
#include "fs/xp/xpconstants.h"
#include "fs/common/magdecreader.h"
#include "fs/util/fsutil.h"
#include "geo/pos.h"
#include "atools.h"

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
  COURSE = 4,
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

XpHoldingWriter::XpHoldingWriter(atools::sql::SqlDatabase& sqlDb,
                                 atools::fs::common::AirportIndex *airportIndexParam,
                                 const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                 atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

XpHoldingWriter::~XpHoldingWriter()
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
void XpHoldingWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  // Avoid duplicates having the exact same values for all fields
  QString key = line.join(",");
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
    airportId = airportIndex->getAirportId(airportIdent);
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
      navType = "W";
      break;

    case HOLD_NDB:
      fetchNdb(navIdent, region, navId, magvar, pos);
      navType = "N";
      break;

    case HOLD_VOR:
      fetchVor(navIdent, region, navId, magvar, pos, vorType, vorDmeOnly, vorHasDme);
      navType = "V";
      break;
  }

  insertQuery->bindValue(":holding_id", ++curHoldId);
  insertQuery->bindValue(":file_id", context.curFileId);

  if(airportId != -1)
  {
    insertQuery->bindValue(":airport_id", airportId);
    insertQuery->bindValue(":airport_ident", airportIdent);
  }

  insertQuery->bindValue(":nav_id", navId);
  insertQuery->bindValue(":nav_ident", navIdent);
  insertQuery->bindValue(":nav_type", navType); // N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end

  if(navType == "V")
  {
    insertQuery->bindValue(":vor_type", vorType);
    insertQuery->bindValue(":vor_dme_only", vorDmeOnly);
    insertQuery->bindValue(":vor_has_dme", vorHasDme);
  }
  else
  {
    insertQuery->bindNullInt(":vor_type");
    insertQuery->bindNullInt(":vor_dme_only");
    insertQuery->bindNullInt(":vor_has_dme");
  }

  insertQuery->bindValue(":region", region);
  insertQuery->bindValue(":mag_var", magvar);
  insertQuery->bindValue(":course", at(line, COURSE).toFloat());
  insertQuery->bindValue(":turn_direction", at(line, DIR));
  insertQuery->bindValue(":leg_length", at(line, LEG_LENGTH).toFloat());
  insertQuery->bindValue(":leg_time", at(line, LEG_TIME).toFloat());
  insertQuery->bindValue(":minimum_altitude", at(line, MIN_ALT).toFloat());
  insertQuery->bindValue(":maximum_altitude", at(line, MAX_ALT).toFloat());
  insertQuery->bindValue(":speed_limit", at(line, SPEED).toInt());
  insertQuery->bindValue(":lonx", pos.getLonX());
  insertQuery->bindValue(":laty", pos.getLatY());
  insertQuery->exec();
  insertQuery->clearBoundValues();
}

void XpHoldingWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertQuery = new SqlQuery(db);
  insertQuery->prepare(util.buildInsertStatement("holding", QString(), {"name"}));

  initNavQueries();
}

void XpHoldingWriter::deInitQueries()
{
  deInitNavQueries();

  delete insertQuery;
  insertQuery = nullptr;
}

void XpHoldingWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context)
}

void XpHoldingWriter::reset()
{
  holdingSet.clear();
}

} // namespace xp
} // namespace fs
} // namespace atools
