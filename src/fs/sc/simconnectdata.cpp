/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

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
    readString(in, result.requestIdent);

    float lonx, laty, altitude;
    quint32 minSinceEpoch;
    in >> lonx >> laty >> altitude >> minSinceEpoch;
    result.requestPos.setAltitude(altitude);
    result.requestPos.setLonX(lonx);
    result.requestPos.setLatY(laty);
    result.timestamp = QDateTime::fromMSecsSinceEpoch(minSinceEpoch * 1000);

    readLongString(in, result.metarForStation);
    readLongString(in, result.metarForNearest);
    readLongString(in, result.metarForInterpolated);

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
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

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
    writeString(out, result.requestIdent);
    out << result.requestPos.getLonX() << result.requestPos.getLatY() << result.requestPos.getAltitude()
        << static_cast<quint32>(result.timestamp.currentMSecsSinceEpoch() / 1000);
    writeLongString(out, result.metarForStation);
    writeLongString(out, result.metarForNearest);
    writeLongString(out, result.metarForInterpolated);
  }

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_DATA));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_DATA));
  out << static_cast<quint32>(size);

  return SimConnectDataBase::writeBlock(ioDevice, block, status);
}

SimConnectData SimConnectData::buildDebugForPosition(const geo::Pos& pos, const geo::Pos& lastPos)
{
  static QVector<float> lastHdgs;
  lastHdgs.fill(0.f, 10);

  SimConnectData data;
  data.userAircraft.position = pos;

  if(lastPos.isValid())
  {

    float h = !lastPos.almostEqual(pos, atools::geo::Pos::POS_EPSILON_10M) ? lastPos.angleDegTo(pos) : 0.f;
    data.userAircraft.headingMag =
      data.userAircraft.headingTrue =
        data.userAircraft.trackMag =
          data.userAircraft.trackTrue =
            h;

    data.userAircraft.groundSpeed = data.userAircraft.indicatedSpeed = data.userAircraft.trueSpeed = 200.f;
  }
  else
  {
    data.userAircraft.headingMag =
      data.userAircraft.headingTrue =
        data.userAircraft.trackMag =
          data.userAircraft.trackTrue =
            0.f;
  }

  data.userAircraft.category = AIRPLANE;
  data.userAircraft.engineType = PISTON;
  data.userAircraft.zuluDateTime = QDateTime::currentDateTimeUtc();
  data.userAircraft.localDateTime = QDateTime::currentDateTime();

  data.userAircraft.airplaneTitle = "Title";
  data.userAircraft.airplaneType = "Type";
  data.userAircraft.airplaneModel = "Model";
  data.userAircraft.airplaneReg = "Ref";
  data.userAircraft.airplaneAirline = "Airline";
  data.userAircraft.airplaneFlightnumber = "965";
  data.userAircraft.fromIdent = "EDDF";
  data.userAircraft.toIdent = "LIRF";
  data.userAircraft.debug = true;

  return data;
}

} // namespace sc
} // namespace fs
} // namespace atools
