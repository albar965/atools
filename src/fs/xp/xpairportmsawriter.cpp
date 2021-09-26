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

#include "fs/xp/xpairportmsawriter.h"

#include "fs/common/airportindex.h"
#include "fs/xp/xpconstants.h"
#include "fs/progresshandler.h"
#include "fs/common/magdecreader.h"
#include "fs/util/fsutil.h"
#include "geo/pos.h"
#include "atools.h"
#include "fs/common/binarymsageometry.h"

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
  TYPE = 0,
  IDENT = 1,
  REGION = 2,
  AIRPORT_IDENT = 3,
  MAG_TRUE = 4,
  BEARING = 5,
  ALTITUDE = 6,
  RADIUS = 7
           /* more BEARING/ALTITUDE/RADIUS values... */
};

/* Point/navaid type for MSA center */
enum MsaType
{
  MSA_FIX = 11,
  MSA_NDB = 2,
  MSA_VOR = 3,
  MSA_AIRPORT = 1,
  MSA_RW_END = 10
};

XpAirportMsaWriter::XpAirportMsaWriter(atools::sql::SqlDatabase& sqlDb,
                                       atools::fs::common::AirportIndex *airportIndexParam,
                                       const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                       atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

XpAirportMsaWriter::~XpAirportMsaWriter()
{
  deInitQueries();
}

// airport_msa_id INTEGER      PRIMARY KEY,
// airport_id     INTEGER,
// airport_ident  VARCHAR (5)  NOT NULL,
// nav_id         INTEGER,
// nav_ident      VARCHAR (5)  NOT NULL,
// region         VARCHAR (2),
// multiple_code  VARCHAR (1),
// type           VARCHAR (15),
// mag_var        DOUBLE,
// left_lonx      DOUBLE,
// top_laty       DOUBLE,
// right_lonx     DOUBLE,
// bottom_laty    DOUBLE,
// radius         DOUBLE       NOT NULL,
// lonx           DOUBLE       NOT NULL,
// laty           DOUBLE       NOT NULL,
// geometry       BLOB,
void XpAirportMsaWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  atools::fs::common::BinaryMsaGeometry geo;

  // 3   BSA DA DAAD M 270 076 25 090 053 25 000 000  0
  // 2   BSA DA DAAD M 270 065 25 090 052 25 000 000  0
  // 3   BJA DA DAAE M 250 087 25 000 076 25 110 063 25 000 000  0
  // 2   BJA DA DAAE M 270 086 25 000 076 25 090 073 25 000 000  0
  QString airportIdent = at(line, AIRPORT_IDENT);
  QString navIdent = at(line, IDENT);

  // Bail out if airport does not exist which happes too often to report
  int airportId = airportIndex->getAirportId(airportIdent);
  if(airportId == -1)
    // qWarning() << context.messagePrefix() << airportIdent << "Airport not found";
    return;

  // Airport center position
  Pos airportPos = airportIndex->getAirportPos(airportIdent);

  QString region = at(line, REGION);
  bool trueBearing = at(line, MAG_TRUE) == "T";

  int navId = -1;
  float magvar = 0.f;
  Pos center;

  // Fetch the center fix by ident and region to get id and coordinates
  MsaType type = static_cast<MsaType>(at(line, TYPE).toInt());
  QString navType;
  switch(type)
  {
    case MSA_AIRPORT:
      navId = airportId;
      navIdent = airportIdent;
      magvar = context.magDecReader->getMagVar(airportPos);
      center = airportPos;
      navType = "A";
      break;

    case MSA_FIX:
      fetchNavaid(waypointQuery, navIdent, region, navId, magvar, center);
      navType = "W";
      break;

    case MSA_NDB:
      fetchNavaid(ndbQuery, navIdent, region, navId, magvar, center);
      navType = "N";
      break;

    case MSA_VOR:
      fetchNavaid(vorQuery, navIdent, region, navId, magvar, center);
      navType = "V";
      break;

    case MSA_RW_END:
      navIdent = navIdent.mid(2);
      navId = airportIndex->getRunwayEndId(airportIdent, navIdent);
      if(navId == -1)
      {
        // Runway end not found - try variants lile 11C and 13C for a 12C
        for(const QString& rw : atools::fs::util::runwayNameVariants(navIdent))
        {
          navId = airportIndex->getRunwayEndId(airportIdent, rw);
          if(navId != -1)
          {
            // Found runway end - replace name with variant
            navIdent = rw;
            break;
          }
        }
      }

      center = airportIndex->getRunwayEndPos(airportIdent, navIdent);
      magvar = context.magDecReader->getMagVar(airportPos);
      navType = "R";
      break;
  }

  if(center.isValid())
  {
    geo.clear();

    // Collect bearing values until all are null ====================
    float radius = 0.f;
    for(int i = BEARING; i < line.size(); i += 3)
    {
      int brg = at(line, i).toInt();
      int alt = at(line, i + 1).toInt();
      float r = at(line, i + 2).toFloat();

      if(brg == 0 && alt == 0 && atools::almostEqual(r, 0.f))
        break;

      geo.addSector(brg, alt * 100.f);

      if(!(radius > 0.f))
        radius = r;
      else if(atools::almostNotEqual(r, radius))
        qWarning() << context.messagePrefix() << airportIdent << "More than one radius found";
    }

    // Calculate geometry for arcs, label points and bearing endpoints to speed up drawing
    geo.calculate(center, radius, trueBearing ? 0.f : magvar);

    if(geo.isValid())
    {
      insertQuery->bindValue(":airport_msa_id", ++curMsaId);
      insertQuery->bindValue(":file_id", context.curFileId);
      insertQuery->bindValue(":airport_id", airportId);
      insertQuery->bindValue(":airport_ident", airportIdent);
      insertQuery->bindValue(":nav_id", navId);
      insertQuery->bindValue(":nav_ident", navIdent);
      insertQuery->bindValue(":region", region);

      insertQuery->bindValue(":type", navType); // N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end
      insertQuery->bindValue(":mag_var", magvar);
      insertQuery->bindValue(":radius", radius);

      // Store bounding rect to simplify queries
      const geo::Rect& bounding = geo.getBoundingRect();
      insertQuery->bindValue(":left_lonx", bounding.getTopLeft().getLonX());
      insertQuery->bindValue(":top_laty", bounding.getTopLeft().getLatY());
      insertQuery->bindValue(":right_lonx", bounding.getBottomRight().getLonX());
      insertQuery->bindValue(":bottom_laty", bounding.getBottomRight().getLatY());

      insertQuery->bindValue(":lonx", center.getLonX());
      insertQuery->bindValue(":laty", center.getLatY());

      insertQuery->bindValue(":geometry", geo.writeToByteArray());

      insertQuery->exec();
    }
    else
      qWarning() << context.messagePrefix() << airportIdent << navIdent << "Invalid MSA geometry";
  }
  else
    qWarning() << context.messagePrefix() << airportIdent << navIdent << "Invalid MSA center coordinate";
}

void XpAirportMsaWriter::fetchNavaid(atools::sql::SqlQuery *query, const QString& ident, const QString& region,
                                     int& id, float& magvar, atools::geo::Pos& pos)
{
  query->bindValue(":ident", ident);
  query->bindValue(":region", region);
  query->exec();
  if(query->next())
  {
    id = query->valueInt("id");
    magvar = query->valueFloat("mag_var");
    pos = Pos(query->valueFloat("lonx"), query->valueFloat("laty"));
  }
  else
  {
    id = -1;
    magvar = 0.f;
    pos = atools::geo::EMPTY_POS;
  }

  query->finish();
}

void XpAirportMsaWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertQuery = new SqlQuery(db);
  insertQuery->prepare(util.buildInsertStatement("airport_msa", QString(), {"multiple_code"}));

  waypointQuery = new SqlQuery(db);
  waypointQuery->prepare("select waypoint_id as id, mag_var, lonx, laty from waypoint "
                         "where ident = :ident and region = :region limit 1");

  ndbQuery = new SqlQuery(db);
  ndbQuery->prepare("select ndb_id as id, mag_var, lonx, laty from ndb "
                    "where ident = :ident and region = :region limit 1");

  vorQuery = new SqlQuery(db);
  vorQuery->prepare("select vor_id as id, mag_var, lonx, laty from vor "
                    "where ident = :ident and region = :region limit 1");
}

void XpAirportMsaWriter::deInitQueries()
{
  delete insertQuery;
  insertQuery = nullptr;

  delete waypointQuery;
  waypointQuery = nullptr;

  delete ndbQuery;
  ndbQuery = nullptr;

  delete vorQuery;
  vorQuery = nullptr;
}

void XpAirportMsaWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context)
}

void XpAirportMsaWriter::reset()
{

}

} // namespace xp
} // namespace fs
} // namespace atools
