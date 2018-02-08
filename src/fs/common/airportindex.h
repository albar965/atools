/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_XPAIRPORTINDEX_H
#define ATOOLS_XPAIRPORTINDEX_H

#include <QHash>
#include <QSet>
#include <QVariant>

namespace atools {
namespace fs {
namespace common {

// Helper classes to avoid QString and speed up the hash maps
class IndexName
{
public:
  explicit IndexName(const QString& str);
  IndexName();

private:
  friend bool operator==(const IndexName& name1, const IndexName& name2);

  friend uint qHash(const IndexName& name);

  static constexpr int SIZE = 10;
  char name[SIZE];
};

inline bool operator==(const IndexName& name1, const IndexName& name2)
{
  return memcmp(name1.name, name2.name, sizeof(name1.name)) == 0;
}

inline bool operator!=(const IndexName& name1, const IndexName& name2)
{
  return !operator==(name1, name2);
}

class IndexName2
{
public:
  explicit IndexName2(const QString& str1, const QString& str2);
  IndexName2();

private:
  friend bool operator==(const IndexName2& name1, const IndexName2& name2);

  friend uint qHash(const IndexName2& name);

  static constexpr int SIZE = 20;
  char name[SIZE];
};

inline bool operator==(const IndexName2& name1, const IndexName2& name2)
{
  return memcmp(name1.name, name2.name, sizeof(name1.name)) == 0;
}

inline bool operator!=(const IndexName2& name1, const IndexName2& name2)
{
  return !operator==(name1, name2);
}

/*
 * Filled when reading airports in the beginning of the compilation process.
 * Provides an index from airport ICAO to airport_id and runwayname/airport ICAO to runway_end_id.
 */
class AirportIndex
{
public:
  AirportIndex();

  /* Get id packed in a variant or a null integer variant if not found.
   *  Returns null if airport id is ENRT */
  QVariant getAirportId(const QString& airportIcao);
  QVariant getRunwayEndId(const QString& airportIcao, const QString& runwayName);

  /* Add airport id to index. Returns true if it is not already indexed and was not added. */
  bool addAirport(const QString& airportIcao, int airportId);
  void addRunwayEnd(const QString& airportIcao, const QString& runwayName, int runwayEndId);

  void addAirportIls(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent, int ilsId);
  int getAirportIlsId(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent);

  void addSkippedAirportIls(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent);
  bool hasSkippedAirportIls(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent);

  void clearSkippedIls()
  {
    skippedIlsSet.clear();
  }

  void clear()
  {
    icaoToIdMap.clear();
    icaoRunwayNameToEndId.clear();
    airportIlsIdMap.clear();
  }

private:
  // Map ICAO id to database airport_id
  QHash<IndexName, int> icaoToIdMap;
  QHash<QString, int> airportIlsIdMap;
  QSet<QString> skippedIlsSet;
  QHash<IndexName2, int> icaoRunwayNameToEndId;

};

} // namespace common
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::common::IndexName, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_XPAIRPORTINDEX_H
