/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/sc/weatherrequest.h"

#include "fs/sc/simconnectdatabase.h"

#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

WeatherRequest::WeatherRequest()
{

}

bool WeatherRequest::isValid() const
{
  return !station.isEmpty() || position.isValid();
}

void WeatherRequest::read(QDataStream& in)
{
  quint8 hasWeather;
  in >> hasWeather;

  if(hasWeather)
  {
    SimConnectDataBase::readString(in, station);

    float lonx, laty, altitude;
    in >> lonx >> laty >> altitude;
    position.setAltitude(altitude);
    position.setLonX(lonx);
    position.setLatY(laty);
  }
  else
  {
    station.clear();
    position = atools::geo::EMPTY_POS;
  }
}

void WeatherRequest::write(QDataStream& out)
{
  quint8 hasWeather = isValid();

  out << hasWeather;
  if(hasWeather)
  {
    SimConnectDataBase::writeString(out, station);
    out << position.getLonX() << position.getLatY() << position.getAltitude();
  }
}

} // namespace sc
} // namespace fs
} // namespace atools
