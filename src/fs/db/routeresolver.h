/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef WRITER_ROUTERESOLVER_H_
#define WRITER_ROUTERESOLVER_H_

#include "sql/sqlquery.h"
#include "geo/pos.h"

#include <QSet>

namespace db {
class Database;
}

namespace atools {
namespace fs {
namespace db {

class RouteResolver
{
public:
  RouteResolver(atools::sql::SqlDatabase& sqlDb);
  virtual ~RouteResolver();

  void run();

  struct Leg
  {
    Leg()
    {

    }

    Leg(int fromId, int toId, int minAltitude, QString routeType,
        const atools::geo::Pos& fromPosition, const atools::geo::Pos& toPosition)
      : from(fromId), to(toId), minAlt(minAltitude), type(routeType), fromPos(fromPosition), toPos(toPosition)
    {
    }

    bool operator==(const Leg& other) const
    {
      return from == other.from && to == other.to;
    }

    bool operator<(const Leg& other) const
    {
      return std::pair<int, int>(from, to) < std::pair<int, int>(other.from, other.to);
    }

    friend QDebug operator<<(QDebug out, const Leg& l);

    int from = 0, to = 0, minAlt = 0;
    QString type;
    atools::geo::Pos fromPos, toPos;
  };

private:
  void writeRoute(const QString& routeName, QSet<atools::fs::db::RouteResolver::Leg>& route);

  int curRouteId, numRoutes;
  atools::sql::SqlQuery routeInsertStmt;
  atools::sql::SqlDatabase& db;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_ROUTERESOLVER_H_ */
