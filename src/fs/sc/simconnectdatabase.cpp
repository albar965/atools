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

#include "fs/sc/simconnectdatabase.h"

#include <QIODevice>
#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectDataBase::SimConnectDataBase()
{

}

SimConnectDataBase::~SimConnectDataBase()
{

}

void SimConnectDataBase::writeString(QDataStream& out, const QString& str) const
{
  // Write string as an size prefixed character array
  QByteArray strBytes;
  strBytes.append(str);
  out << static_cast<quint16>(strBytes.size());
  out << str;
}

bool SimConnectDataBase::readString(QDataStream& in, QString& str, quint16 *size)
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

int SimConnectDataBase::writeBlock(QIODevice *ioDevice, const QByteArray& block, SimConnectStatus& status)
{
  qint64 written = ioDevice->write(block);

  if(written < block.size())
  {
    qWarning() << "SimConnectData::write: wrote only" << written << "of" << block.size();
    status = INSUFFICIENT_WRITE;
  }

  // qint64 written = 0;
  // const char *data = block.data();

  // while(written < block.size())
  // {
  // qint64 wrote = ioDevice->write(data + written, block.size() - written);

  // if(wrote == -1)
  // {
  // qWarning() << "SimConnectAirplane::write: error" << ioDevice->errorString();
  // status = WRITE_ERROR;
  // return static_cast<int>(written);
  // }

  // written += wrote;
  // }
  return static_cast<int>(written);
}

} // namespace sc
} // namespace fs
} // namespace atools
