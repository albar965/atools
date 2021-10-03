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

#include "fs/xp/xpwriter.h"
#include "fs/navdatabaseerrors.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace xp {

XpWriter::XpWriter(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : db(sqlDb), options(opts), progress(progressHandler), errors(navdatabaseErrors)
{

}

XpWriter::~XpWriter()
{

}

void XpWriter::fetchWaypoint(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos)
{
  fetchNavaid(waypointQuery, ident, region, id, magvar, pos);
}

void XpWriter::fetchNdb(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos)
{
  fetchNavaid(ndbQuery, ident, region, id, magvar, pos);
}

void XpWriter::fetchVor(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos,
                        QString& vorType, bool& dmeOnly, bool& hasDme)
{
  fetchNavaid(vorQuery, ident, region, id, magvar, pos, &vorType, &dmeOnly, &hasDme);
}

void XpWriter::fetchNavaid(atools::sql::SqlQuery *query, const QString& ident, const QString& region,
                           int& id, float& magvar, atools::geo::Pos& pos, QString *vorType,
                           bool *dmeOnly, bool *hasDme)
{
  query->bindValue(":ident", ident);
  query->bindValue(":region", region);
  query->exec();
  if(query->next())
  {
    id = query->valueInt("id");
    magvar = query->valueFloat("mag_var");
    pos = atools::geo::Pos(query->valueFloat("lonx"), query->valueFloat("laty"));

    if(vorType != nullptr)
      *vorType = query->valueStr("type");

    if(dmeOnly != nullptr)
      *dmeOnly = query->valueInt("dme_only");

    if(hasDme != nullptr)
      *hasDme = !query->isNull("dme_altitude");
  }
  else
  {
    id = -1;
    magvar = 0.f;
    pos = atools::geo::EMPTY_POS;
  }

  query->finish();
}

void XpWriter::initNavQueries()
{
  deInitNavQueries();

  atools::sql::SqlUtil util(&db);

  waypointQuery = new atools::sql::SqlQuery(db);
  waypointQuery->prepare("select waypoint_id as id, mag_var, lonx, laty from waypoint "
                         "where ident = :ident and region = :region limit 1");

  ndbQuery = new atools::sql::SqlQuery(db);
  ndbQuery->prepare("select ndb_id as id, mag_var, lonx, laty from ndb "
                    "where ident = :ident and region = :region limit 1");

  vorQuery = new atools::sql::SqlQuery(db);
  vorQuery->prepare("select vor_id as id, type, dme_only, dme_altitude, mag_var, lonx, laty from vor "
                    "where ident = :ident and region = :region limit 1");
}

void XpWriter::deInitNavQueries()
{
  delete waypointQuery;
  waypointQuery = nullptr;

  delete ndbQuery;
  ndbQuery = nullptr;

  delete vorQuery;
  vorQuery = nullptr;
}

void XpWriter::err(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;

  if(errors != nullptr)
  {
    if(!errors->sceneryErrors.isEmpty())
    {
      atools::fs::NavDatabaseErrors::SceneryFileError err;
      if(ctx != nullptr)
      {
        err.errorMessage = ctx->messagePrefix() + " " + msg;
        err.filepath = ctx->filePath;
      }
      else
        err.errorMessage = msg;
      err.lineNum = 0;

      errors->sceneryErrors.first().fileErrors.append(err);
    }
  }
}

void XpWriter::errWarn(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;
}

} // namespace xp
} // namespace fs
} // namespace atools
