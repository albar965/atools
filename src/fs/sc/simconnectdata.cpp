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

#include "logging/loggingdefs.h"
#include "fs/sc/simconnectdata.h"

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

  quint16 intFlags;
  in >> intFlags;
  flags = Flags(intFlags);

  readString(in, airplaneTitle);
  readString(in, airplaneModel);
  readString(in, airplaneReg);
  readString(in, airplaneType);
  readString(in, airplaneAirline);
  readString(in, airplaneFlightnumber);

  float lonx, laty, altitude;
  in >> lonx >> laty >> altitude >> headingTrue >> headingMag
  >> groundSpeed >> indicatedSpeed >> windSpeed >> windDirection >> verticalSpeed
  >> indicatedAltitude >> altitudeAboveGround >> groundAltitude >> trueSpeed >> machSpeed
  >> trackMag >> trackTrue >> ambientTemperature >> totalAirTemperature >> seaLevelPressure
  >> pitotIce >> structuralIce >> airplaneTotalWeight >> airplaneMaxGrossWeight >> airplaneEmptyWeight
  >> fuelTotalQuantity >> fuelTotalWeight >> fuelFlowPPH >> fuelFlowGPH >> magVar >> ambientVisibility
  >> localDateTime >> zuluDateTime;

  position.setAltitude(altitude);
  position.setLonX(lonx);
  position.setLatY(laty);

  return true;
}

int SimConnectData::write(QIODevice *ioDevice)
{
  status = OK;

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);

  out << MAGIC_NUMBER_DATA << packetSize << DATA_VERSION << packetId << packetTs;
  out << static_cast<quint16>(flags);

  writeString(out, airplaneTitle);
  writeString(out, airplaneModel);
  writeString(out, airplaneReg);
  writeString(out, airplaneType);
  writeString(out, airplaneAirline);
  writeString(out, airplaneFlightnumber);

  out << position.getLonX() << position.getLatY() << position.getAltitude() << headingTrue << headingMag
      << groundSpeed << indicatedSpeed << windSpeed << windDirection << verticalSpeed
      << indicatedAltitude << altitudeAboveGround << groundAltitude << trueSpeed << machSpeed
      << trackMag << trackTrue << ambientTemperature << totalAirTemperature << seaLevelPressure
      << pitotIce << structuralIce << airplaneTotalWeight << airplaneMaxGrossWeight << airplaneEmptyWeight
      << fuelTotalQuantity << fuelTotalWeight << fuelFlowPPH << fuelFlowGPH << magVar << ambientVisibility
      << localDateTime << zuluDateTime;

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_DATA));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_DATA));
  out << static_cast<quint16>(size);

  qint64 written = ioDevice->write(block);

  if(written < block.size())
  {
    qWarning() << "SimConnectData::write: wrote only" << written << "of" << block.size();
    status = INSUFFICIENT_WRITE;
  }

  return static_cast<int>(written);
}

void SimConnectData::writeString(QDataStream& out, const QString& str) const
{
  // Write string as an size prefixed character array
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

} // namespace sc
} // namespace fs
} // namespace atools
