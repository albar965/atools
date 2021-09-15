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

#include "fs/common/airportindex.h"
#include "geo/pos.h"

namespace atools {
namespace fs {
namespace common {

const static QLatin1String EN_ROUTE("ENRT");
const static QVariant NULL_INT(QVariant::Int);

// ==========================================================================
AirportIndex::AirportIndex()
{

}

bool AirportIndex::addAirportIdent(const QString& airportIdent)
{
  Name idxName(airportIdent);
  if(airportIdents.contains(idxName))
    return false;

  airportIdents.insert(idxName);
  return true;
}

QVariant AirportIndex::getAirportId(const QString& airportIdent) const
{
  if(!airportIdent.isEmpty() && airportIdent != EN_ROUTE)
  {
    int id = identToIdMap.value(Name(airportIdent), -1);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

atools::geo::Pos AirportIndex::getAirportPos(const QString& airportIdent) const
{
  if(!airportIdent.isEmpty() && airportIdent != EN_ROUTE)
    return identToPosMap.value(Name(airportIdent), atools::geo::EMPTY_POS);

  return atools::geo::EMPTY_POS;
}

QVariant AirportIndex::getRunwayEndId(const QString& airportIdent, const QString& runwayName) const
{
  if(!airportIdent.isEmpty())
  {
    int id = identRunwayNameToEndId.value(Name2(airportIdent, runwayName), -1);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

atools::geo::Pos AirportIndex::getRunwayEndPos(const QString& airportIdent, const QString& runwayName) const
{
  if(!airportIdent.isEmpty())
    return identRunwayNameToEndPos.value(Name2(airportIdent, runwayName), atools::geo::EMPTY_POS);

  return atools::geo::EMPTY_POS;
}

bool AirportIndex::addAirportId(const QString& airportIdent, int airportId, const geo::Pos& pos)
{
  if(identToIdMap.contains(Name(airportIdent)))
    return false;
  else
  {
    identToIdMap.insert(Name(airportIdent), airportId);
    identToPosMap.insert(Name(airportIdent), pos);
    return true;
  }
}

void AirportIndex::addRunwayEnd(const QString& airportIdent, const QString& runwayName, int runwayEndId,
                                const geo::Pos& runwayEndPos)
{
  identRunwayNameToEndId.insert(Name2(airportIdent, runwayName), runwayEndId);
  identRunwayNameToEndPos.insert(Name2(airportIdent, runwayName), runwayEndPos);
}

void AirportIndex::addAirportIls(const QString& airportIdent, const QString& airportRegion, const QString& ilsIdent,
                                 int ilsId)
{
  airportIlsIdMap.insert(Name3(airportIdent, airportRegion, ilsIdent), ilsId);
}

int AirportIndex::getAirportIlsId(const QString& airportIdent, const QString& airportRegion,
                                  const QString& ilsIdent) const
{
  return airportIlsIdMap.value(Name3(airportIdent, airportRegion, ilsIdent), -1);
}

void AirportIndex::addSkippedAirportIls(const QString& airportIdent, const QString& airportRegion,
                                        const QString& ilsIdent)
{
  skippedIlsSet.insert(Name3(airportIdent, airportRegion, ilsIdent));
}

bool AirportIndex::hasSkippedAirportIls(const QString& airportIdent, const QString& airportRegion,
                                        const QString& ilsIdent) const
{
  return skippedIlsSet.contains(Name3(airportIdent, airportRegion, ilsIdent));
}

bool AirportIndex::addIdentIcaoMapping(const QString& airportIdent, const QString& icao)
{
  bool retval = false;
  if(!airportIdent.isEmpty() && !icao.isEmpty())
  {
    retval = identToIcaoMap.contains(Name(icao));
    identToIcaoMap.insert(Name(airportIdent), Name(icao));
  }
  return retval;
}

void AirportIndex::clear()
{
  identToIdMap.clear();
  identToPosMap.clear();
  airportIdents.clear();
  identToIcaoMap.clear();
  identRunwayNameToEndId.clear();
  identRunwayNameToEndPos.clear();
  airportIlsIdMap.clear();
}

} // namespace common
} // namespace fs
} // namespace atools
