/*
 * RouteResolver.h
 *
 *  Created on: 12.05.2015
 *      Author: alex
 */

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
  RouteResolver(atools::sql::SqlDatabase& db);
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
      return std::make_pair(from, to) < std::make_pair(l.from, l.to);
    }

    friend QDebug operator<<(QDebug out, const Leg& l);

    int from;
    int to;
    int minAlt;
    QString type;
  };

private:
  void writeRoute(const QString& routeName, QSet<Leg>& route);

  atools::sql::SqlQuery routeInsertStmt;

  int curRouteId, numRoutes;

  atools::sql::SqlDatabase& db;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_ROUTERESOLVER_H_ */
