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

#include "fs/sc/simconnectdata.h"

#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectData::SimConnectData()
{

}

SimConnectData::SimConnectData(const SimConnectData& other)
{
  *this = other;
}

SimConnectData::~SimConnectData()
{

}

bool SimConnectData::read(QIODevice *ioDevice)
{
  status = OK;

  QDataStream in(ioDevice);
  in.setVersion(QDataStream::Qt_5_5);

  if(magicNumber == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(magicNumber)))
      return false;

    in >> magicNumber;
    if(magicNumber != MAGIC_NUMBER_DATA)
    {
      qWarning() << "SimConnectData::read: invalid magic number" << magicNumber;
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
  if(version != DATA_VERSION)
  {
    qWarning() << "SimConnectData::read: version mismatch" << version << "!=" << DATA_VERSION;
    status = VERSION_MISMATCH;
    return false;
  }
  in >> packetId >> packetTs;

  quint8 hasUser = 0;
  in >> hasUser;
  if(hasUser == 1)
    userAircraft.read(in);

  quint16 numAi = 0;
  in >> numAi;
  for(quint16 i = 0; i < numAi; i++)
  {
    SimConnectAircraft ap;
    ap.read(in);
    aiAircraft.append(ap);
  }

  quint16 numMetar = 0;
  in >> numMetar;
  for(quint16 i = 0; i < numMetar; i++)
  {

    MetarResult result;
    readString(in, result.metarIdent);

    float lonx, laty, altitude;
    in >> lonx >> laty >> altitude;
    result.metarPos.setAltitude(altitude);
    result.metarPos.setLonX(lonx);
    result.metarPos.setLatY(laty);

    readLongString(in, result.metar);

    metarResults.append(result);
  }

  return true;
}

int SimConnectData::write(QIODevice *ioDevice)
{
  status = OK;

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);

  out << MAGIC_NUMBER_DATA << packetSize << DATA_VERSION << packetId << packetTs;

  bool userValid = userAircraft.getPosition().isValid();
  out << static_cast<quint8>(userValid);
  if(userValid)
    userAircraft.write(out);

  int numAi = std::min(65535, aiAircraft.size());
  out << static_cast<quint16>(numAi);

  for(int i = 0; i < numAi; i++)
    aiAircraft.at(i).write(out);

  int numMetar = std::min(65535, metarResults.size());
  out << static_cast<quint16>(numMetar);

  for(int i = 0; i < numMetar; i++)
  {
    const MetarResult& result = metarResults.at(i);
    writeString(out, result.metarIdent);
    out << result.metarPos.getLonX() << result.metarPos.getLatY() << result.metarPos.getAltitude();
    writeLongString(out, result.metar);
  }

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_DATA));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_DATA));
  out << static_cast<quint32>(size);

  return SimConnectDataBase::writeBlock(ioDevice, block, status);
}

} // namespace sc
} // namespace fs
} // namespace atools
