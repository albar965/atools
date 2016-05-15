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

#ifndef ATOOLS_FS_SIMCONNECTDATA_H
#define ATOOLS_FS_SIMCONNECTDATA_H

#include <QString>
#include <geo/pos.h>
#include "fs/simconnectdata.h"

class QIODevice;

namespace atools {
namespace fs {

class SimConnectData
{
public:
  SimConnectData();

  bool read(QIODevice *ioDevice);

  int write(QIODevice *ioDevice) const;

  const QString& getAirplaneTitle() const
  {
    return airplaneTitle;
  }

  void setAirplaneTitle(const QString& value)
  {
    airplaneTitle = value;
  }

  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  void setAirplaneModel(const QString& value)
  {
    airplaneModel = value;
  }

  const QString& getAirplaneReg() const
  {
    return airplaneReg;
  }

  void setAirplaneReg(const QString& value)
  {
    airplaneReg = value;
  }

  int getPacketId() const
  {
    return static_cast<int>(packetId);
  }

  void setPacketId(int value)
  {
    packetId = static_cast<quint32>(value);
  }

  int getPacketTs() const
  {
    return static_cast<int>(packetTs);
  }

  void setPacketTs(unsigned int value)
  {
    packetTs = static_cast<quint32>(value);
  }

  atools::geo::Pos& getPosition()
  {
    return position;
  }

  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  void setPosition(const atools::geo::Pos& value)
  {
    position = value;
  }

  float getCourseTrue() const
  {
    return courseTrue;
  }

  void setCourseTrue(float value)
  {
    courseTrue = value;
  }

  float getCourseMag() const
  {
    return courseMag;
  }

  void setCourseMag(float value)
  {
    courseMag = value;
  }

  float getGroundSpeed() const
  {
    return groundSpeed;
  }

  void setGroundSpeed(float value)
  {
    groundSpeed = value;
  }

  float getIndicatedSpeed() const
  {
    return indicatedSpeed;
  }

  void setIndicatedSpeed(float value)
  {
    indicatedSpeed = value;
  }

  float getWindSpeed() const
  {
    return windSpeed;
  }

  void setWindSpeed(float value)
  {
    windSpeed = value;
  }

  float getWindDirection() const
  {
    return windDirection;
  }

  void setWindDirection(float value)
  {
    windDirection = value;
  }

  float getVerticalSpeed() const
  {
    return verticalSpeed;
  }

  void setVerticalSpeed(float value)
  {
    verticalSpeed = value;
  }

  const QString& getAirplaneType() const
  {
    return airplaneType;
  }

  void setAirplaneType(const QString& value)
  {
    airplaneType = value;
  }

  const QString& getAirplaneAirline() const
  {
    return airplaneAirline;
  }

  void setAirplaneAirline(const QString& value)
  {
    airplaneAirline = value;
  }

  const QString& getAirplaneFlightnumber() const
  {
    return airplaneFlightnumber;
  }

  void setAirplaneFlightnumber(const QString& value)
  {
    airplaneFlightnumber = value;
  }

private:
  void writeString(QDataStream& out, const QString& str) const;
  bool readString(QDataStream& in, QString& str, quint16 *size = nullptr);

  quint32 packetSize = 0, packetId = 0, packetTs = 0, version = 1;
  QString airplaneTitle, airplaneModel, airplaneReg, airplaneType,
          airplaneAirline, airplaneFlightnumber;

  atools::geo::Pos position;
  float courseTrue = 0.f, courseMag = 0.f, groundSpeed = 0.f, indicatedSpeed = 0.f, windSpeed = 0.f,
        windDirection = 0.f, verticalSpeed = 0.f;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTDATA_H
