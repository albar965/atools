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
#include <QSet>

namespace db {
class Database;
}

namespace atools {
namespace fs {
namespace writer {

class RouteResolver
{
public:
  RouteResolver(atools::sql::SqlDatabase& sqlDb);
  virtual ~RouteResolver();

  void run();

  class Leg
  {
public:
    Leg()
      : from(0), to(0), minAlt(0), type("")
    {
    }

    Leg(int from, int to, int minAlt, QString type)
      : from(from), to(to), minAlt(minAlt), type(type)
    {
    }

    bool operator==(const Leg& l) const
    {
      return from == l.from && to == l.to;
    }

    bool operator<(const Leg& l) const
    {
      return QPair<int, int>(from, to) < QPair<int, int>(l.from, l.to);
    }

    friend QDebug operator<<(QDebug out, const Leg& l);

    int from;
    int to;
    int minAlt;
    QString type;
  };

private:
  void writeRoute(const QString& routeName, QSet<atools::fs::writer::RouteResolver::Leg>& route);

  int curRouteId, numRoutes;
  atools::sql::SqlQuery routeInsertStmt;
  atools::sql::SqlDatabase& db;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_ROUTERESOLVER_H_ */
