/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

static QLatin1String EN_ROUTE("ENRT");

IndexName::IndexName(const QString& str)
{
  for(int i = 0; i < SIZE; i++)
  {
    if(i < str.size())
      name[i] = str.at(i).toLatin1();
    else
      name[i] = ' ';
  }
}

IndexName::IndexName()
{
  memset(name, ' ', SIZE);
}

uint qHash(const IndexName& name)
{
  int retval = 0;
  for(int i = 0; i < IndexName::SIZE; i++)
    retval ^= name.name[i] << ((i % 4) * 8);

  return static_cast<uint>(retval);
}

IndexName2::IndexName2(const QString& str1, const QString& str2)
{
  for(int i = 0; i < SIZE / 2; i++)
  {
    if(i < str1.size())
      name[i] = str1.at(i).toLatin1();
    else
      name[i] = ' ';
  }
  for(int i = 0; i < SIZE / 2; i++)
  {
    if(i < str2.size())
      name[i + SIZE / 2] = str2.at(i).toLatin1();
    else
      name[i + SIZE / 2] = ' ';
  }
}

IndexName2::IndexName2()
{
  memset(name, ' ', SIZE);
}

uint qHash(const IndexName2& name)
{
  int retval = 0;
  for(int i = 0; i < IndexName2::SIZE; i++)
    retval ^= name.name[i] << ((i % 4) * 8);

  return static_cast<uint>(retval);
}

// ==========================================================================
AirportIndex::AirportIndex()
{

}

QVariant AirportIndex::getAirportId(const QString& airportIcao)
{
  if(airportIcao != EN_ROUTE)
  {
    int id = icaoToIdMap.value(IndexName(airportIcao), -1);
    if(id != -1)
      return id;
  }
  return QVariant(QVariant::Int);
}

QVariant AirportIndex::getRunwayEndId(const QString& airportIcao, const QString& runwayName)
{
  if(airportIcao != EN_ROUTE) // en route
  {
    int id = icaoRunwayNameToEndId.value(IndexName2(airportIcao, runwayName), -1);
    if(id != -1)
      return id;
  }
  return QVariant(QVariant::Int);
}

bool AirportIndex::addAirport(const QString& airportIcao, int airportId)
{
  if(icaoToIdMap.contains(IndexName(airportIcao)))
    return false;
  else
  {
    icaoToIdMap.insert(IndexName(airportIcao), airportId);
    return true;
  }
}

void AirportIndex::addRunwayEnd(const QString& airportIcao, const QString& runwayName, int runwayEndId)
{
  icaoRunwayNameToEndId.insert(IndexName2(airportIcao, runwayName), runwayEndId);
}

void AirportIndex::addAirportIls(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent,
                                 int ilsId)
{
  airportIlsIdMap.insert(QString("%1|%2|%3").arg(airportIcao).arg(airportRegion).arg(ilsIdent), ilsId);
}

int AirportIndex::getAirportIlsId(const QString& airportIcao, const QString& airportRegion, const QString& ilsIdent)
{
  return airportIlsIdMap.value(QString("%1|%2|%3").arg(airportIcao).arg(airportRegion).arg(ilsIdent), -1);
}

void AirportIndex::addSkippedAirportIls(const QString& airportIcao, const QString& airportRegion,
                                        const QString& ilsIdent)
{
  skippedIlsSet.insert(QString("%1|%2|%3").arg(airportIcao).arg(airportRegion).arg(ilsIdent));
}

bool AirportIndex::hasSkippedAirportIls(const QString& airportIcao, const QString& airportRegion,
                                        const QString& ilsIdent)
{
  return skippedIlsSet.contains(QString("%1|%2|%3").arg(airportIcao).arg(airportRegion).arg(ilsIdent));
}

} // namespace common
} // namespace fs
} // namespace atools
