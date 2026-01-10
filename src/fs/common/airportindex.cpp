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

#include "fs/common/airportindex.h"

#include "geo/pos.h"
#include "fs/util/fsutil.h"

namespace atools {
namespace fs {
namespace common {

const static QLatin1String EN_ROUTE("ENRT");
const static QVariant NULL_INT(QMetaType::fromType<int>());

const static AirportIndex::IdPos EMPTY_IDPOS = std::make_pair(-1, atools::geo::EMPTY_POS);
const static AirportIndex::IdName EMPTY_IDNAME = std::make_pair(-1, AirportIndex::Name());

bool AirportIndex::addAirportIdent(const QString& ident)
{
  Name idxName(ident);
  if(airportIdents.contains(idxName))
    return false;

  airportIdents.insert(idxName);
  return true;
}

QVariant AirportIndex::getAirportIdVar(const QString& ident, bool allIdents) const
{
  if(!ident.isEmpty() && ident != EN_ROUTE)
  {
    int id = getAirportId(ident, allIdents);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

int AirportIndex::getAirportId(const QString& ident, bool allIdents) const
{
  if(ident.isEmpty() || ident == EN_ROUTE)
    return -1;

  int id = identToAirportMap.value(Name(ident), EMPTY_IDPOS).first;

  if(allIdents)
  {
    if(id == -1)
      id = icaoToAirportMap.value(Name(ident), EMPTY_IDPOS).first;

    if(id == -1)
      id = faaToAirportMap.value(Name(ident), EMPTY_IDPOS).first;

    if(id == -1)
      id = localToAirportMap.value(Name(ident), EMPTY_IDPOS).first;
  }

  return id;
}

atools::geo::Pos AirportIndex::getAirportPos(const QString& ident, bool allIdents) const
{
  if(ident.isEmpty() || ident == EN_ROUTE)
    return atools::geo::EMPTY_POS;

  atools::geo::Pos pos = identToAirportMap.value(Name(ident), EMPTY_IDPOS).second;

  if(allIdents)
  {
    if(!pos.isValid())
      pos = icaoToAirportMap.value(Name(ident), EMPTY_IDPOS).second;

    if(!pos.isValid())
      pos = faaToAirportMap.value(Name(ident), EMPTY_IDPOS).second;

    if(!pos.isValid())
      pos = localToAirportMap.value(Name(ident), EMPTY_IDPOS).second;
  }

  return pos;
}

QVariant AirportIndex::getRunwayEndIdVar(int airportId, const QString& runwayName) const
{
  if(airportId != -1)
  {
    int id = runwayEndId(airportId, runwayName);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

int AirportIndex::runwayEndId(int airportId, const QString& runwayName) const
{
  return idNameToEnd.value(IdName(airportId, Name(util::normalizeRunway(runwayName))), EMPTY_IDPOS).first;
}

atools::geo::Pos AirportIndex::getRunwayEndPos(const QString& airportIdent, const QString& runwayName, bool allAirportIdents) const
{
  if(!airportIdent.isEmpty())
    return idNameToEnd.value(IdName(getAirportId(airportIdent, allAirportIdents), Name(util::normalizeRunway(runwayName))),
                             EMPTY_IDPOS).second;

  return atools::geo::EMPTY_POS;
}

bool AirportIndex::addAirportId(const QString& ident, const QString& icao, const QString& faa, const QString& local, int airportId,
                                const geo::Pos& pos)
{
  if(identToAirportMap.contains(Name(ident)))
    return false;
  else
  {
    const IdPos idPos(airportId, pos);
    identToAirportMap.insert(Name(ident), idPos);

    if(!icao.isEmpty())
      icaoToAirportMap.insert(Name(icao), idPos);

    if(!faa.isEmpty())
      faaToAirportMap.insert(Name(faa), idPos);

    if(!local.isEmpty())
      localToAirportMap.insert(Name(local), idPos);

    return true;
  }
}

void AirportIndex::addRunwayEnd(int airportId, const QString& runwayName, int runwayEndId, const geo::Pos& runwayEndPos)
{
  idNameToEnd.insert(IdName(airportId, util::normalizeRunway(runwayName)), IdPos(runwayEndId, runwayEndPos));
}

void AirportIndex::addAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent, int ilsId)
{
  airportIlsIdMap.insert(Name3(airportIdent, airportRegion, ilsIdent), ilsId);
}

int AirportIndex::getAirportIlsId(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent) const
{
  return airportIlsIdMap.value(Name3(airportIdent, airportRegion, ilsIdent), -1);
}

void AirportIndex::addSkippedAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent)
{
  skippedIlsSet.insert(Name3(airportIdent, airportRegion, ilsIdent));
}

bool AirportIndex::hasSkippedAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent) const
{
  return skippedIlsSet.contains(Name3(airportIdent, airportRegion, ilsIdent));
}

void AirportIndex::clear()
{
  identToAirportMap.clear();
  icaoToAirportMap.clear();
  faaToAirportMap.clear();
  localToAirportMap.clear();
  airportIdents.clear();
  idNameToEnd.clear();
  airportIlsIdMap.clear();
}

} // namespace common
} // namespace fs
} // namespace atools
