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
#include <QVariant>

namespace atools {
namespace fs {
namespace xp {

class XpAirportIndex
{
public:
  XpAirportIndex();

  /* Get id packed in a variant or a null integer variant if not found */
  QVariant getAirportId(const QString& icao);
  QVariant getRunwayEndId(const QString& airportIcao, const QString& runwayName);

  /* Add airport id to index. Returns true if it is not already indexed and was not added. */
  bool addAirport(const QString& airportIcao, int airportId);
  void addRunwayEnd(const QString& airportIcao, const QString& runwayName, int runwayEndId);

  void clear()
  {
    icaoToIdMap.clear();
    icaoRunwayNameToEndId.clear();
  }

private:
  // Map ICAO id to database airport_id
  QHash<QString, int> icaoToIdMap;
  QHash<std::pair<QString, QString>, int> icaoRunwayNameToEndId;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPAIRPORTINDEX_H
