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

#include "fs/sc/weatherrequest.h"

#include "fs/sc/simconnectdatabase.h"

#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

WeatherRequest::WeatherRequest()
{

}

WeatherRequest::WeatherRequest(const WeatherRequest& other)
{
  *this = other;
}

WeatherRequest::~WeatherRequest()
{

}

const QVector<atools::geo::Pos>& WeatherRequest::getWeatherRequestInterpolated() const
{
  return weatherRequestInterpolated;
}

void WeatherRequest::setWeatherRequestInterpolated(const QVector<atools::geo::Pos>& value)
{
  weatherRequestInterpolated = value;
}

const QStringList& WeatherRequest::getWeatherRequestStation() const
{
  return weatherRequestStation;
}

void WeatherRequest::setWeatherRequestStation(const QStringList& value)
{
  weatherRequestStation = value;
}

bool WeatherRequest::isValid() const
{
  return !weatherRequestNearest.isEmpty() || !weatherRequestInterpolated.isEmpty() ||
         !weatherRequestStation.isEmpty();
}

const QVector<atools::geo::Pos>& WeatherRequest::getWeatherRequestNearest() const
{
  return weatherRequestNearest;
}

void WeatherRequest::setWeatherRequestNearest(const QVector<atools::geo::Pos>& value)
{
  weatherRequestNearest = value;
}

void WeatherRequest::read(QDataStream& in)
{
  quint8 numWeather;
  float lonx, laty, altitude;
  in >> numWeather;
  for(int i = 0; i < numWeather; i++)
  {
    in >> lonx >> laty >> altitude;
    weatherRequestNearest.append(atools::geo::Pos(lonx, laty, altitude));
  }

  in >> numWeather;
  for(int i = 0; i < numWeather; i++)
  {
    in >> lonx >> laty >> altitude;
    weatherRequestInterpolated.append(atools::geo::Pos(lonx, laty, altitude));
  }

  in >> numWeather;
  QString station;
  for(int i = 0; i < numWeather; i++)
  {
    SimConnectDataBase::readString(in, station);
    weatherRequestStation.append(station);
  }
}

void WeatherRequest::write(QDataStream& out)
{
  quint8 numWeather = static_cast<quint8>(weatherRequestNearest.size());
  out << numWeather;
  for(const atools::geo::Pos& pos : weatherRequestNearest)
    out << pos.getLonX() << pos.getLatY() << pos.getAltitude();

  numWeather = static_cast<quint8>(weatherRequestInterpolated.size());
  out << numWeather;
  for(const atools::geo::Pos& pos : weatherRequestInterpolated)
    out << pos.getLonX() << pos.getLatY() << pos.getAltitude();

  numWeather = static_cast<quint8>(weatherRequestStation.size());
  out << numWeather;
  for(const QString& station : weatherRequestStation)
    SimConnectDataBase::writeString(out, station);
}

} // namespace sc
} // namespace fs
} // namespace atools
