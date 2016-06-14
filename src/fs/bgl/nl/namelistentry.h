/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_NAMELISTICAOIDENT_H
#define ATOOLS_BGL_NAMELISTICAOIDENT_H

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

class NamelistEntry
{
public:
  NamelistEntry()
  {
  }

  virtual ~NamelistEntry();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  const QString& getAirportName() const
  {
    return airportName;
  }

  const QString& getCityName() const
  {
    return cityName;
  }

  const QString& getCountryName() const
  {
    return countryName;
  }

  const QString& getRegionIdent() const
  {
    return regionIdent;
  }

  const QString& getRegionName() const
  {
    return regionName;
  }

  const QString& getStateName() const
  {
    return stateName;
  }

private:
  friend class Namelist;
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::NamelistEntry& record);

  QString regionName, countryName, stateName, cityName, airportName, airportIdent, regionIdent;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAMELISTICAOIDENT_H
