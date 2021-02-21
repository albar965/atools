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

#ifndef ATOOLS_FS_SC_WEATHERREQUEST_H
#define ATOOLS_FS_SC_WEATHERREQUEST_H

#include "geo/pos.h"

#include <QVector>

namespace atools {
namespace fs {
namespace sc {

class WeatherRequest
{
public:
  WeatherRequest();
  WeatherRequest(const WeatherRequest& other);

  void read(QDataStream & in);
  void write(QDataStream& out);

  bool isValid() const;

  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  void setPosition(const atools::geo::Pos& value)
  {
    position = value;
  }

  const QString& getStation() const
  {
    return station;
  }

  void setStation(const QString& value)
  {
    station = value;
  }

private:
  QString station;
  atools::geo::Pos position;

};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::WeatherRequest);

#endif // ATOOLS_FS_SC_WEATHERREQUEST_H
