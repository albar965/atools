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

#ifndef ATOOLS_FS_XP_WRITER_H
#define ATOOLS_FS_XP_WRITER_H

#include "exception.h"
#include "fs/xp/xpconstants.h"

#include <QStringList>

class QStringList;

namespace atools {
namespace geo {
class Pos;
}
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace xp {

struct XpWriterContext;

/*
 * Base class for all X-Plane reading classes to read dat files.
 */
class XpWriter
{
public:
  XpWriter(atools::sql::SqlDatabase& sqlDb,
           const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
           atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpWriter();

  /* Called for each line read from a dat file */
  virtual void write(const QStringList& line, const atools::fs::xp::XpWriterContext& context) = 0;

  /* Called when finished with reading a dat file */
  virtual void finish(const atools::fs::xp::XpWriterContext& context) = 0;

  /* Reset all internal states/caches etc. */
  virtual void reset() = 0;

protected:
  /* Called very often - make inline. Throws exception if index is out of bounds */
  const QString& at(const QStringList& line, int index)
  {
    if(index < line.size())
      return line.at(index);
    else
      // Have to stop reading the file since the rest can be corrupted
      throw atools::Exception(ctx->messagePrefix() +
                              QString(": Index out of bounds: Index: %1, size: %2").arg(index).arg(line.size()));
  }

  QString mid(const QStringList& line, int index, bool ignoreError = false)
  {
    if(index < line.size())
      return line.mid(index).join(" ");
    else if(!ignoreError)
      // Have to stop reading the file since the rest can be corrupted
      throw atools::Exception(ctx->messagePrefix() +
                              QString(": Index out of bounds: Index: %1, size: %2").arg(index).arg(line.size()));
    return QString();
  }

  /* Report error in log without throwing an exception */
  void err(const QString& msg);

  /* Report error to log file without throwing an exception */
  void errWarn(const QString& msg);

  void initNavQueries();
  void deInitNavQueries();

  const atools::fs::xp::XpWriterContext *ctx = nullptr;
  atools::sql::SqlDatabase& db;
  const atools::fs::NavDatabaseOptions& options;
  atools::fs::ProgressHandler *progress;
  atools::fs::NavDatabaseErrors *errors = nullptr;

  void fetchWaypoint(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos);
  void fetchNdb(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos);
  void fetchVor(const QString& ident, const QString& region, int& id, float& magvar, atools::geo::Pos& pos,
                QString& vorType, bool& dmeOnly, bool& hasDme);

private:
  // Fetch id, magvar and coordinates for a navaid by ident and region
  void fetchNavaid(atools::sql::SqlQuery *query, const QString& ident, const QString& region, int& id, float& magvar,
                   atools::geo::Pos& pos, QString *vorType = nullptr, bool *dmeOnly = nullptr, bool *hasDme = nullptr);

  atools::sql::SqlQuery *waypointQuery = nullptr, *ndbQuery = nullptr, *vorQuery = nullptr;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_WRITER_H
