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
  virtual ~WeatherRequest();

  void read(QDataStream& in);
  void write(QDataStream& out);

  const QVector<atools::geo::Pos>& getWeatherRequestNearest() const;
  void setWeatherRequestNearest(const QVector<atools::geo::Pos>& value);

  const QVector<atools::geo::Pos>& getWeatherRequestInterpolated() const;
  void setWeatherRequestInterpolated(const QVector<atools::geo::Pos>& value);

  const QStringList& getWeatherRequestStation() const;
  void setWeatherRequestStation(const QStringList& value);

  bool isValid() const;

private:
  QVector<atools::geo::Pos> weatherRequestNearest;
  QVector<atools::geo::Pos> weatherRequestInterpolated;
  QStringList weatherRequestStation;

};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::WeatherRequest);

Q_DECLARE_TYPEINFO(atools::fs::sc::WeatherRequest, Q_MOVABLE_TYPE);

#endif // ATOOLS_FS_SC_WEATHERREQUEST_H
