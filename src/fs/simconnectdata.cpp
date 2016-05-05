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

#include "simconnectdata.h"
#include "logging/loggingdefs.h"

#include <QDataStream>

namespace atools {
namespace fs {

SimConnectData::SimConnectData()
{

}

bool SimConnectData::read(QIODevice *ioDevice)
{
  QDataStream in(ioDevice);
  in.setVersion(QDataStream::Qt_5_5);

  if(packetSize == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(packetSize)))
      return false;

    in >> packetSize;
  }

  // Wait until the whole packet is available
  if(ioDevice->bytesAvailable() < packetSize)
    return false;

  in >> packetId >> packetTs >> version; // TODO version check

  readString(in, airplaneName, airplaneNameSize);
  readString(in, airplaneType, airplaneTypeSize);
  readString(in, airplaneReg, airplaneRegSize);

  float lonx, laty, altitude;
  in >> lonx >> laty >> altitude >> courseTrue >> courseMag
  >> groundSpeed >> indicatedSpeed >> windSpeed >> windDirection;

  position.setAltitude(altitude);
  position.setLonX(lonx);
  position.setLatY(laty);

  return true;
}

void SimConnectData::write(QIODevice *ioDevice) const
{
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);

  out << static_cast<quint32>(0); // packetSize will be updated later
  out << packetId << packetTs << version;

  writeString(out, airplaneName);
  writeString(out, airplaneType);
  writeString(out, airplaneReg);

  out << position.getLonX() << position.getLatY() << position.getAltitude() << courseTrue << courseMag
      << groundSpeed << indicatedSpeed << windSpeed << windDirection;

  // Go back and update size
  out.device()->seek(0);
  out << static_cast<quint32>(block.size()) - static_cast<quint32>(sizeof(packetSize));

  ioDevice->write(block);
}

























void SimConnectData::writeString(QDataStream& out, const QString& str) const
{
  QByteArray strBytes;
  strBytes.append(str);
  out << static_cast<quint16>(strBytes.size());
  out << str;
}

bool SimConnectData::readString(QDataStream& in, QString& str, quint16& size)
{
  if(size == 0)
  {
    if(in.device()->bytesAvailable() < static_cast<qint64>(sizeof(size)))
      return false;

    in >> size;
  }

  if(in.device()->bytesAvailable() < size)
    return false;

  in >> str;
  return true;
}

} // namespace fs
} // namespace atools
