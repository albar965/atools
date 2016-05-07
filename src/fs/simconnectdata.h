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

  void write(QIODevice *ioDevice) const;

  const QString& getAirplaneName() const
  {
    return airplaneName;
  }

  void setAirplaneName(const QString& value)
  {
    airplaneName = value;
  }

  const QString& getAirplaneType() const
  {
    return airplaneType;
  }

  void setAirplaneType(const QString& value)
  {
    airplaneType = value;
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
    return static_cast<float>(courseTrue);
  }

  void setCourseTrue(float value)
  {
    courseTrue = value;
  }

  float getCourseMag() const
  {
    return static_cast<float>(courseMag);
  }

  void setCourseMag(float value)
  {
    courseMag = value;
  }

  float getGroundSpeed() const
  {
    return static_cast<float>(groundSpeed);
  }

  void setGroundSpeed(float value)
  {
    groundSpeed = value;
  }

  float getIndicatedSpeed() const
  {
    return static_cast<float>(indicatedSpeed);
  }

  void setIndicatedSpeed(float value)
  {
    indicatedSpeed = value;
  }

  float getWindSpeed() const
  {
    return static_cast<float>(windSpeed);
  }

  void setWindSpeed(float value)
  {
    windSpeed = value;
  }

  float getWindDirection() const
  {
    return static_cast<float>(windDirection);
  }

  void setWindDirection(float value)
  {
    windDirection = value;
  }

  float getVerticalSpeed() const
  {
    return static_cast<float>(verticalSpeed);
  }

  void setVerticalSpeed(float value)
  {
    verticalSpeed = value;
  }

private:
  void writeString(QDataStream& out, const QString& str) const;
  bool readString(QDataStream& in, QString& str, quint16& size);

  quint32 packetSize = 0, packetId = 0, packetTs = 0, version = 1;
  QString airplaneName, airplaneType, airplaneReg;
  quint16 airplaneNameSize = 0, airplaneTypeSize = 0, airplaneRegSize = 0;

  atools::geo::Pos position;
  qreal courseTrue = 0.f, courseMag = 0.f, groundSpeed = 0.f, indicatedSpeed = 0.f, windSpeed = 0.f,
        windDirection = 0.f, verticalSpeed = 0.f;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTDATA_H
