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
  // Write string as an size prefixed character array max length 256 UTF-8
  QByteArray strBytes;
  strBytes.append(str);
  if(strBytes.size() > 255)
    strBytes = strBytes.left(255);

  // Size does not include the trailing 0
  out << static_cast<quint8>(strBytes.size());
  out.writeRawData(strBytes.constData(), strBytes.size());
}

bool SimConnectDataBase::readString(QDataStream& in, QString& str)
{
  quint8 size = 0;

  if(in.device()->bytesAvailable() < static_cast<qint64>(sizeof(quint8)))
    return false;

  in >> size;

  if(in.device()->bytesAvailable() < size)
    return false;

  char *buffer = new char[size + 1];
  in.readRawData(buffer, size);
  buffer[size] = '\0';

  str = QString(buffer);
  delete[] buffer;

  return true;
}

int SimConnectDataBase::writeBlock(QIODevice *ioDevice, const QByteArray& block, SimConnectStatus& status)
{
  qint64 written = ioDevice->write(block);

  // We can write the whole block since ioDevice is buffered
  if(written < block.size())
  {
    qWarning() << "SimConnectData::write: wrote only" << written << "of" << block.size();
    status = INSUFFICIENT_WRITE;
  }

  return static_cast<int>(written);
}

} // namespace sc
} // namespace fs
} // namespace atools
