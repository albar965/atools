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

static QLatin1String EN_ROUTE("ENRT");

IndexName::IndexName(const QString& str)
{
  for(int i = 0; i < 4; i++)
  {
    if(i < str.size())
      name[i] = str.at(i).toLatin1();
    else
      name[i] = '\0';
  }
}

IndexName::IndexName()
{
  for(int i = 0; i < 4; i++)
    name[i] = '\0';
}

uint qHash(const IndexName& name)
{
  return static_cast<uint>((name.name[3] << 3) ^ (name.name[2] << 2) ^ (name.name[1] << 1) ^ (name.name[0] << 0));
}

bool operator==(const IndexName& name1, const IndexName& name2)
{
  return memcmp(name1.name, name2.name, sizeof(name1.name)) == 0;
}

bool operator!=(const IndexName& name1, const IndexName& name2)
{
  return !operator==(name1, name2);
}

IndexName2::IndexName2(const QString& str1, const QString& str2)
{
  for(int i = 0; i < 4; i++)
  {
    if(i < str1.size())
      name[i] = str1.at(i).toLatin1();
    else
      name[i] = '\0';
  }
  for(int i = 0; i < 4; i++)
  {
    if(i < str2.size())
      name[i + 4] = str2.at(i).toLatin1();
    else
      name[i + 4] = '\0';
  }
}

IndexName2::IndexName2()
{
  for(int i = 0; i < 8; i++)
    name[i] = '\0';
}

uint qHash(const IndexName2& name)
{
  return static_cast<uint>(
    (name.name[3] << 3) ^ (name.name[2] << 2) ^ (name.name[1] << 1) ^ (name.name[0] << 0) ^
    (name.name[7] << 3) ^ (name.name[6] << 2) ^ (name.name[5] << 1) ^ (name.name[4] << 0)
    );
}

bool operator==(const IndexName2& name1, const IndexName2& name2)
{
  return memcmp(name1.name, name2.name, sizeof(name1.name)) == 0;
}

bool operator!=(const IndexName2& name1, const IndexName2& name2)
{
  return !operator==(name1, name2);
}

// ==========================================================================
XpAirportIndex::XpAirportIndex()
{

}

QVariant XpAirportIndex::getAirportId(const QString& airportIcao)
{
  if(airportIcao != EN_ROUTE)
  {
    int id = icaoToIdMap.value(IndexName(airportIcao), -1);
    if(id != -1)
      return id;
  }
  return QVariant(QVariant::Int);
}

QVariant XpAirportIndex::getRunwayEndId(const QString& airportIcao, const QString& runwayName)
{
  if(airportIcao != EN_ROUTE) // en route
  {
    int id = icaoRunwayNameToEndId.value(IndexName2(airportIcao, runwayName), -1);
    if(id != -1)
      return id;
  }
  return QVariant(QVariant::Int);
}

bool atools::fs::xp::XpAirportIndex::addAirport(const QString& airportIcao, int airportId)
{
  if(icaoToIdMap.contains(IndexName(airportIcao)))
    return false;
  else
  {
    icaoToIdMap.insert(IndexName(airportIcao), airportId);
    return true;
  }
}

void XpAirportIndex::addRunwayEnd(const QString& airportIcao, const QString& runwayName, int runwayEndId)
{
  icaoRunwayNameToEndId.insert(IndexName2(airportIcao, runwayName), runwayEndId);
}

} // namespace xp
} // namespace fs
} // namespace atools
