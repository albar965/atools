/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "fs/sc/simconnectreply.h"
#include "fs/sc/simconnectdatabase.h"

#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectReply::SimConnectReply()
{

}

SimConnectReply::SimConnectReply(const SimConnectReply& other)
{
  *this = other;

}

SimConnectReply::~SimConnectReply()
{

}

bool SimConnectReply::read(QIODevice *ioDevice)
{
  QDataStream in(ioDevice);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);
  status = OK;

  if(magicNumber == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(magicNumber)))
      return false;

    in >> magicNumber;
    if(magicNumber != MAGIC_NUMBER_REPLY)
    {
      qWarning() << "SimConnectReply::read: invalid magic number" << magicNumber;
      status = INVALID_MAGIC_NUMBER;
      return false;
    }
  }

  if(packetSize == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(packetSize)))
      return false;

    in >> packetSize;
  }

  // Wait until the whole packet is available
  if(ioDevice->bytesAvailable() < packetSize)
    return false;

  in >> version;
  if(version != REPLY_VERSION)
  {
    qWarning() << "SimConnectReply::read: version mismatch" << version << "!=" << REPLY_VERSION;
    status = VERSION_MISMATCH;
    return false;
  }
  quint16 cmd;
  in >> packetId >> packetTs >> cmd;
  command = Command(cmd);

  weatherRequest.read(in);

  return true;
}

int SimConnectReply::write(QIODevice *ioDevice)
{
  status = OK;

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << MAGIC_NUMBER_REPLY << packetSize << REPLY_VERSION << packetId << packetTs
      << static_cast<quint16>(command);

  weatherRequest.write(out);

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_REPLY));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_REPLY));
  out << static_cast<quint32>(size);

  return SimConnectDataBase::writeBlock(ioDevice, block, status);
}

} // namespace sc
} // namespace fs
} // namespace atools
