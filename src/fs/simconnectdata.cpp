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

  readString(in, airplaneTitle);
  readString(in, airplaneModel);
  readString(in, airplaneReg);
  readString(in, airplaneType);
  readString(in, airplaneAirline);
  readString(in, airplaneFlightnumber);

  float lonx, laty, altitude;
  in >> lonx >> laty >> altitude >> courseTrue >> courseMag
  >> groundSpeed >> indicatedSpeed >> windSpeed >> windDirection >> verticalSpeed;

  position.setAltitude(altitude);
  position.setLonX(lonx);
  position.setLatY(laty);

  return true;
}

int SimConnectData::write(QIODevice *ioDevice) const
{
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);

  out << static_cast<quint32>(0); // packetSize will be updated later
  out << packetId << packetTs << version;

  writeString(out, airplaneTitle);
  writeString(out, airplaneModel);
  writeString(out, airplaneReg);
  writeString(out, airplaneType);
  writeString(out, airplaneAirline);
  writeString(out, airplaneFlightnumber);

  out << position.getLonX() << position.getLatY() << position.getAltitude() << courseTrue << courseMag
      << groundSpeed << indicatedSpeed << windSpeed << windDirection << verticalSpeed;

  // Go back and update size
  out.device()->seek(0);
  int size = block.size() - static_cast<int>(sizeof(packetSize));
  out << static_cast<quint32>(size);

  ioDevice->write(block);
  return block.size();
}

void SimConnectData::writeString(QDataStream& out, const QString& str) const
{
  QByteArray strBytes;
  strBytes.append(str);
  out << static_cast<quint16>(strBytes.size());
  out << str;
}

bool SimConnectData::readString(QDataStream& in, QString& str, quint16 *size)
{
  quint16 localSize = 0;
  quint16 *sizePtr = size != nullptr ? size : &localSize;

  if(*sizePtr == 0)
  {
    if(in.device()->bytesAvailable() < static_cast<qint64>(sizeof(quint16)))
      return false;

    in >> *sizePtr;
  }

  if(in.device()->bytesAvailable() < *sizePtr)
    return false;

  in >> str;
  return true;
}

} // namespace fs
} // namespace atools
