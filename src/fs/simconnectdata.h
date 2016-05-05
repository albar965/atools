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

  quint32 getPacketId() const
  {
    return packetId;
  }

  void setPacketId(quint32 value)
  {
    packetId = value;
  }

  quint32 getPacketTs() const
  {
    return packetTs;
  }

  void setPacketTs(quint32 value)
  {
    packetTs = value;
  }

private:
  quint32 packetSize = 0, packetId = 0, packetTs = 0, version = 1;
  QString airplaneName, airplaneType, airplaneReg;
  quint16 airplaneNameSize = 0, airplaneTypeSize = 0, airplaneRegSize = 0;
  qreal lonx = 0.f, laty = 0.f, altitude = 0.f, courseTrue = 0.f, courseMag = 0.f,
        groundSpeed = 0.f, indicatedSpeed = 0.f, windSpeed = 0.f, windDirection = 0.f;

  void writeString(QDataStream& out, const QString& str) const;
  bool readString(QDataStream& in, QString& str, quint16& size);

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTDATA_H
