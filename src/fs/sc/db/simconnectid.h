/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_SIMCONNECTIDFACILITIES_H
#define ATOOLS_SIMCONNECTIDFACILITIES_H

#include "geo/pos.h"

#include <QVector>
#include <QDebug>

namespace atools {
namespace fs {
namespace sc {
namespace db {

/* =============================================================================================
 * Simple efficient id string which can be hashed, compared and printed to qDebug.
 * Maximum of 9 characters for ICAO/ident and region plus terminating zero.
 */
class IcaoId
{
public:
  IcaoId()
  {
    ident[0] = '\0';
  }

  explicit IcaoId(const QString& identParam)
  {
    init(identParam.toLatin1().constData());
  }

  explicit IcaoId(const char *identParam)
  {
    init(identParam);
  }

  bool operator<(const IcaoId& other) const
  {
    return strcmp(ident, other.ident) < 0;
  }

  bool operator!=(const IcaoId& other) const
  {
    return !(*this == other);
  }

  bool operator==(const IcaoId& other) const
  {
    return strcmp(ident, other.ident) == 0;
  }

  bool isValid() const
  {
    return strlen(ident) > 0;
  }

  QString getIdentStr() const
  {
    return QString(ident);
  }

  const char *getIdent() const
  {
    return ident;
  }

private:
  friend uint qHash(const IcaoId& str);

  void init(const char *identParam)
  {
    if(identParam != nullptr)
    {
      strncpy(ident, identParam, SIZE - 1);
      ident[SIZE - 1] = '\0';
    }
    else
      ident[0] = '\0';
  }

  const static int SIZE = 10;
  char ident[SIZE];
};

inline uint qHash(const IcaoId& id)
{
  return qHash(QLatin1String(id.ident));
}

typedef QList<IcaoId> IcaoIdList;
typedef QVector<IcaoId> IcaoIdVector;
typedef QSet<IcaoId> IcaoIdSet;

QDebug operator<<(QDebug out, const atools::fs::sc::db::IcaoId& obj);

/* Type of FacilityId */
enum IdType : char
{
  ID_NONE = '\0',
  ID_WAYPOINT = 'W',
  ID_VORILS = 'V',
  ID_NDB = 'N',
  ID_AIPORT = 'A',
  ID_RUNWAY = 'R'
};

/* =============================================================================================
 * Simple efficient id string combining ICAO/ident, region and type which can be hashed,
 * compared and printed to qDebug.
 * Sort order is by ident, region and type.
 * Maximum of 9 characters for ICAO/ident and region plus terminating zero.
 * Strings are copied on construction.
 */
class FacilityId
{
public:
  FacilityId()
  {
    ident[0] = region[0] = '\0';
  }

  explicit FacilityId(const QString& identParam, const QString& regionParam = QString(), QChar typeParam = ID_NONE,
                      float lonXParam = atools::geo::Pos::INVALID_VALUE, float latYParam = atools::geo::Pos::INVALID_VALUE)
    : type(static_cast<IdType>(typeParam.toLatin1())), lonX(lonXParam), latY(latYParam)
  {
    init(identParam.toLatin1().constData(), regionParam.toLatin1().constData());
  }

  explicit FacilityId(const char *identParam, const char *regionParam = nullptr, IdType typeParam = ID_NONE,
                      float lonXParam = atools::geo::Pos::INVALID_VALUE, float latYParam = atools::geo::Pos::INVALID_VALUE)
    : type(typeParam), lonX(lonXParam), latY(latYParam)
  {
    init(identParam, regionParam);
  }

  explicit FacilityId(const QString& identParam, const QString& regionParam, QChar typeParam, const atools::geo::Pos& pos)
    : type(static_cast<IdType>(typeParam.toLatin1())), lonX(pos.getLonX()), latY(pos.getLatY())
  {
    init(identParam.toLatin1().constData(), regionParam.toLatin1().constData());
  }

  explicit FacilityId(const char *identParam, const char *regionParam, IdType typeParam, const atools::geo::Pos& pos)
    : type(typeParam), lonX(pos.getLonX()), latY(pos.getLatY())
  {
    init(identParam, regionParam);
  }

  FacilityId noRegion() const
  {
    FacilityId id(*this);
    id.region[0] = '\0';
    return id;
  }

  bool operator<(const FacilityId& other) const
  {
    if(strcmp(ident, other.ident) == 0)
    {
      if(strcmp(region, other.region) == 0)
        return type == other.type;
      else
        return strcmp(region, other.region) < 0;
    }
    else
      return strcmp(ident, other.ident) < 0;
  }

  bool operator!=(const FacilityId& other) const
  {
    return !(*this == other);
  }

  bool operator==(const FacilityId& other) const
  {
    return strcmp(ident, other.ident) == 0 && strcmp(region, other.region) == 0 && type == other.type;
  }

  bool isValid() const
  {
    return strlen(ident) > 0;
  }

  IdType getType() const
  {
    return type;
  }

  QString getTypeStr() const
  {
    return QString(QChar(type));
  }

  QString getRegionStr() const
  {
    return QString(region);
  }

  QString getIdentStr() const
  {
    return QString(ident);
  }

  const char *getRegion() const
  {
    return region;
  }

  const char *getIdent() const
  {
    return ident;
  }

  atools::geo::Pos getPos() const
  {
    return atools::geo::Pos(lonX, latY);
  }

  float getLonX() const
  {
    return lonX;
  }

  float getLatY() const
  {
    return latY;
  }

private:
  friend uint qHash(const FacilityId& str);

  void init(const char *identParam, const char *regionParam)
  {
    if(identParam != nullptr)
    {
      strncpy(ident, identParam, SIZE - 1);
      ident[SIZE - 1] = '\0';
    }
    else
      ident[0] = '\0';

    if(regionParam != nullptr)
    {
      strncpy(region, regionParam, SIZE - 1);
      region[SIZE - 1] = '\0';
    }
    else
      region[0] = '\0';
  }

  const static int SIZE = 10;
  IdType type = ID_NONE;
  char ident[SIZE], region[SIZE];
  float lonX, latY;
};

inline uint qHash(const FacilityId& id)
{
  return qHash(QLatin1String(id.ident)) ^ qHash(QLatin1String(id.region)) ^ static_cast<int>(id.type);
}

typedef QVector<FacilityId> FacilityIdVector;
typedef QList<FacilityId> FacilityIdList;
typedef QSet<FacilityId> FacilityIdSet;

QDebug operator<<(QDebug out, const atools::fs::sc::db::FacilityId& obj);

/* =============================================================================================
 * Simple efficient id string combining airport ICAO/ident, airport region, runway name and runway end id.
 * Maximum of 9 characters for ICAO/ident and region plus terminating zero.
 */
class RunwayId
{
public:
  RunwayId()
  {
    airportIdent[0] = airportRegion[0] = runwayName[0] = '\0';
  }

  RunwayId(const QString& airportIdent, const QString& airportRegion, const QString& runwayName, int runwayEndId)
    : runwayEndId(runwayEndId)
  {
    init(airportIdent.toLatin1().constData(), airportRegion.toLatin1().constData(), runwayName.toLatin1().constData());
  }

  int getRunwayEndId() const
  {
    return runwayEndId;
  }

  QString getAirportRegionStr() const
  {
    return QString(airportRegion);
  }

  QString getAirportIdentStr() const
  {
    return QString(airportIdent);
  }

  QString getRunwayStr() const
  {
    return QString(runwayName);
  }

  const char *getAirportRegion() const
  {
    return airportRegion;
  }

  const char *getAirportIdent() const
  {
    return airportIdent;
  }

  const char *getRunway() const
  {
    return runwayName;
  }

  bool isValid() const
  {
    return runwayEndId != -1;
  }

private:
  void init(const char *identParam, const char *regionParam, const char *runwayParam)
  {
    strncpy(airportIdent, identParam, SIZE - 1);
    strncpy(airportRegion, regionParam, SIZE - 1);
    strncpy(runwayName, runwayParam, SIZE - 1);
    airportIdent[SIZE - 1] = airportRegion[SIZE - 1] = runwayName[SIZE - 1] = '\0';
  }

  int runwayEndId = -1;

  const static int SIZE = 10;
  char airportIdent[SIZE], airportRegion[SIZE], runwayName[SIZE];
};

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::sc::db::FacilityId, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(atools::fs::sc::db::IcaoId, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_SIMCONNECTIDFACILITIES_H
