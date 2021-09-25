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

#ifndef ATOOLS_XPAIRPORTINDEX_H
#define ATOOLS_XPAIRPORTINDEX_H

#include "util/str.h"

#include <QHash>
#include <QSet>
#include <QVariant>

namespace atools {
namespace geo {
class Pos;
}

namespace fs {
namespace common {

/*
 * Filled when reading airports in the beginning of the compilation process.
 * Provides an index from airport ICAO to airport_id and runwayname/airport ICAO to runway_end_id.
 */
class AirportIndex
{
public:
  AirportIndex();

  /* Add airport to index. Returns true if it was added. ident should be ICAO for X-Plane.
   *  Uses a different set/hash than getAirportId and addAirportId */
  bool addAirportIdent(const QString& airportIdent);

  /* Add airport id to index. Returns true if it was added. ident should be ICAO for X-Plane. */
  bool addAirportId(const QString& airportIdent, int airportId, const atools::geo::Pos& pos);

  /* Get id packed in a variant or a null integer variant if not found.
   *  Returns null if airport id is ENRT */
  QVariant getAirportIdVar(const QString& airportIdent) const;

  /* As above but -1 for not found */
  int getAirportId(const QString& airportIdent) const;

  /* Airport position or invalid pos if not found */
  atools::geo::Pos getAirportPos(const QString& airportIdent) const;

  /* Add runway end id to index. Returns true if it was added. ident should be ICAO for X-Plane. */
  void addRunwayEnd(const QString& airportIdent, const QString& runwayName, int runwayEndId,
                    const atools::geo::Pos& runwayEndPos);

  /* Get runway end id packed in a variant or a null integer variant if not found.
   *  Returns null if airport id is ENRT */
  QVariant getRunwayEndIdVar(const QString& airportIdent, const QString& runwayName) const;

  /* As above but -1 for not found */
  int getRunwayEndId(const QString& airportIdent, const QString& runwayName) const;

  /* Get position of runway end. Invalid pos if not found */
  atools::geo::Pos getRunwayEndPos(const QString& airportIdent, const QString& runwayName) const;

  void addAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent, int ilsId);
  int getAirportIlsId(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent) const;

  void addSkippedAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent);
  bool hasSkippedAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent) const;

  /* Returns true if ICAO duplicates appear */
  bool addIdentIcaoMapping(const QString& airportIdent, const QString& icao);

  void clearSkippedIls()
  {
    skippedIlsSet.clear();
  }

  void clear();

  typedef atools::util::Str<8> Name;
  typedef atools::util::StrPair<8> Name2;
  typedef atools::util::StrTriple<8> Name3;

private:
  // Airport ICAO to airport_id
  QHash<Name, int> identToIdMap;

  // Airport idents
  QSet<Name> airportIdents;

  // Maps airport idents to ICAO
  QHash<Name, Name> identToIcaoMap;
  QHash<Name, atools::geo::Pos> identToPosMap;

  // Airport ICAO and runway name to runway_end_id
  QHash<Name2, int> identRunwayNameToEndId;

  QHash<Name2, atools::geo::Pos> identRunwayNameToEndPos;

  // Airport ICAO, airport region and ILS ident to ils_id
  QHash<Name3, int> airportIlsIdMap;
  QSet<Name3> skippedIlsSet;

};

} // namespace common
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::common::AirportIndex::Name, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(atools::fs::common::AirportIndex::Name2, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(atools::fs::common::AirportIndex::Name3, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_XPAIRPORTINDEX_H
