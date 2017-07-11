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

#include "fs/xp/xpairportindex.h"

namespace atools {
namespace fs {
namespace xp {

XpAirportIndex::XpAirportIndex()
{

}

QVariant XpAirportIndex::getAirportId(const QString& icao)
{
  if(icao != "ENRT")
  {
    int id = icaoToIdMap.value(icao, -1);
    if(id != -1)
      return id;
  }
  return QVariant(QVariant::Int);
}

QVariant XpAirportIndex::getRunwayEndId(const QString& airportIcao, const QString& runwayName)
{
  int id = icaoRunwayNameToEndId.value(std::make_pair(airportIcao, runwayName), -1);
  if(id == -1)
    return QVariant(QVariant::Int);
  else
    return id;
}

bool atools::fs::xp::XpAirportIndex::addAirport(const QString& airportIcao, int airportId)
{
  if(icaoToIdMap.contains(airportIcao))
    return false;
  else
  {
    icaoToIdMap.insert(airportIcao, airportId);
    return true;
  }
}

void XpAirportIndex::addRunwayEnd(const QString& airportIcao, const QString& runwayName, int runwayEndId)
{
  icaoRunwayNameToEndId.insert(std::make_pair(airportIcao, runwayName), runwayEndId);
}

} // namespace xp
} // namespace fs
} // namespace atools
