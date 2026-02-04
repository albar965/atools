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

#include "fs/xp/xpreader.h"
#include "fs/navdatabaseerrors.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace xp {

XpReader::XpReader(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : db(sqlDb), options(opts), progress(progressHandler), errors(navdatabaseErrors)
{

}

XpReader::~XpReader()
{

}

const QString& XpReader::at(const QStringList& line, int index)
{
  if(index < line.size())
    return line.at(index);
  else
    // Have to stop reading the file since the rest can be corrupted
    throw atools::Exception(ctx->messagePrefix() +
                            QStringLiteral(": Index out of bounds: Index: %1, size: %2").arg(index).arg(line.size()));
}

QString XpReader::atAirportIdent(const QStringList& line, int index)
{
  if(index < line.size())
  {
    const QString str = line.at(index).simplified();
    return str == QStringLiteral("ENRT") ? QString() : str;
  }
  else
    // Have to stop reading the file since the rest can be corrupted
    throw atools::Exception(ctx->messagePrefix() +
                            QStringLiteral(": Index out of bounds: Index: %1, size: %2").arg(index).arg(line.size()));
}

QString XpReader::mid(const QStringList& line, int index, bool ignoreError)
{
  if(index < line.size())
    return line.mid(index).join(QStringLiteral(" "));
  else if(!ignoreError)
    // Have to stop reading the file since the rest can be corrupted
    throw atools::Exception(ctx->messagePrefix() +
                            QStringLiteral(": Index out of bounds: Index: %1, size: %2").arg(index).arg(line.size()));
  return QString();
}

void XpReader::fetchWaypoint(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos)
{
  fetchNavaid(waypointQuery, ident, region, id, magvar, pos);
}

void XpReader::fetchNdb(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos)
{
  fetchNavaid(ndbQuery, ident, region, id, magvar, pos);
}

void XpReader::fetchVor(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos,
                        QString& vorType, bool& dmeOnly, bool& hasDme)
{
  fetchNavaid(vorQuery, ident, region, id, magvar, pos, &vorType, &dmeOnly, &hasDme);
}

void XpReader::fetchIls(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos)
{
  fetchNavaid(ilsQuery, ident, region, id, magvar, pos);
}

void XpReader::fetchNavaid(atools::sql::SqlQuery *query, const QString& ident, const QString& region,
                           int& id, float& magvar, atools::geo::Pos& pos, QString *vorType,
                           bool *dmeOnly, bool *hasDme)
{
  query->bindValue(QStringLiteral(":ident"), ident);
  query->bindValue(QStringLiteral(":region"), region);
  query->exec();
  if(query->next())
  {
    id = query->valueInt(QStringLiteral("id"));
    magvar = query->valueFloat(QStringLiteral("mag_var"));
    pos = atools::geo::Pos(query->valueFloat(QStringLiteral("lonx")), query->valueFloat(QStringLiteral("laty")));

    if(vorType != nullptr)
      *vorType = query->valueStr(QStringLiteral("type"));

    if(dmeOnly != nullptr)
      *dmeOnly = query->valueInt(QStringLiteral("dme_only"));

    if(hasDme != nullptr)
      *hasDme = !query->isNull(QStringLiteral("dme_altitude"));
  }
  else
  {
    id = -1;
    magvar = 0.f;
    pos = atools::geo::EMPTY_POS;
  }

  query->finish();
}

void XpReader::initNavQueries()
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

  ilsQuery = new atools::sql::SqlQuery(db);
  ilsQuery->prepare("select ils_id as id, mag_var, lonx, laty from ils "
                    "where ident = :ident and region = :region limit 1");
}

void XpReader::deInitNavQueries()
{
  delete waypointQuery;
  waypointQuery = nullptr;

  delete ndbQuery;
  ndbQuery = nullptr;

  delete vorQuery;
  vorQuery = nullptr;

  delete ilsQuery;
  ilsQuery = nullptr;
}

void XpReader::err(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;

  if(errors != nullptr)
  {
    if(!errors->getSceneryErrors().isEmpty())
    {
      QString message, filepath;

      if(ctx != nullptr)
      {
        message = ctx->messagePrefix() + QStringLiteral(" ") + msg;
        filepath = ctx->filePath;
      }
      else
        message = msg;

      errors->getSceneryErrors().first().appendFileError(SceneryFileError(filepath, message));
    }
  }
}

void XpReader::errWarn(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;
}

} // namespace xp
} // namespace fs
} // namespace atools
