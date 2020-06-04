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

namespace atools {
namespace fs {
namespace common {

const static QLatin1String EN_ROUTE("ENRT");
const static QVariant NULL_INT(QVariant::Int);

// ==========================================================================
AirportIndex::AirportIndex()
{

}

bool AirportIndex::addAirportIdent(const QString& ident)
{
  Name idxName(ident);
  if(airportIdents.contains(idxName))
    return false;

  airportIdents.insert(idxName);
  return true;
}

QVariant AirportIndex::getAirportId(const QString& ident) const
{
  if(!ident.isEmpty() && ident != EN_ROUTE)
  {
    int id = identToIdMap.value(Name(ident), -1);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

QVariant AirportIndex::getRunwayEndId(const QString& ident, const QString& runwayName) const
{
  QVariant var = NULL_INT;

  if(!ident.isEmpty())
  {
    int id = identRunwayNameToEndId.value(Name2(ident, runwayName), -1);
    if(id != -1)
      return id;
  }
  return NULL_INT;
}

bool AirportIndex::addAirportId(const QString& ident, int airportId)
{
  if(identToIdMap.contains(Name(ident)))
    return false;
  else
  {
    identToIdMap.insert(Name(ident), airportId);
    return true;
  }
}

void AirportIndex::addRunwayEnd(const QString& ident, const QString& runwayName, int runwayEndId)
{
  identRunwayNameToEndId.insert(Name2(ident, runwayName), runwayEndId);
}

void AirportIndex::addAirportIls(const QString& ident, const QString& airportRegion, const QString& ilsIdent,
                                 int ilsId)
{
  airportIlsIdMap.insert(Name3(ident, airportRegion, ilsIdent), ilsId);
}

int AirportIndex::getAirportIlsId(const QString& ident, const QString& airportRegion, const QString& ilsIdent) const
{
  return airportIlsIdMap.value(Name3(ident, airportRegion, ilsIdent), -1);
}

void AirportIndex::addSkippedAirportIls(const QString& ident, const QString& airportRegion,
                                        const QString& ilsIdent)
{
  skippedIlsSet.insert(Name3(ident, airportRegion, ilsIdent));
}

bool AirportIndex::hasSkippedAirportIls(const QString& ident, const QString& airportRegion,
                                        const QString& ilsIdent) const
{
  return skippedIlsSet.contains(Name3(ident, airportRegion, ilsIdent));
}

bool AirportIndex::addIdentIcaoMapping(const QString& ident, const QString& icao)
{
  bool retval = false;
  if(!ident.isEmpty() && !icao.isEmpty())
  {
    retval = identToIcaoMap.contains(Name(icao));
    identToIcaoMap.insert(Name(ident), Name(icao));
  }
  return retval;
}

void AirportIndex::clear()
{
  identToIdMap.clear();
  airportIdents.clear();
  identToIcaoMap.clear();
  identRunwayNameToEndId.clear();
  airportIlsIdMap.clear();
}

} // namespace common
} // namespace fs
} // namespace atools
