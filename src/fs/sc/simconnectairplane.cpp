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

#include "fs/sc/simconnectairplane.h"

#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectAircraft::SimConnectAircraft()
{

}

SimConnectAircraft::SimConnectAircraft(const SimConnectAircraft& other)
{
  *this = other;
}

SimConnectAircraft::~SimConnectAircraft()
{

}

void SimConnectAircraft::read(QDataStream& in)
{
  quint32 intFlags;
  in >> intFlags;
  flags = Flags(intFlags);

  readString(in, airplaneTitle);
  readString(in, airplaneModel);
  readString(in, airplaneReg);
  readString(in, airplaneType);
  readString(in, airplaneAirline);
  readString(in, airplaneFlightnumber);

  float lonx, laty, altitude;
  quint8 categoryByte, engineTypeByte;

  in >> lonx >> laty >> altitude >> headingTrue >> headingMag
  >> groundSpeed >> indicatedSpeed >> verticalSpeed
  >> indicatedAltitude >> trueSpeed >> machSpeed
  >> trackMag >> trackTrue >> magVar >> numberOfEngines >> categoryByte >> engineTypeByte;

  position.setAltitude(altitude);
  position.setLonX(lonx);
  position.setLatY(laty);

  category = static_cast<Category>(categoryByte);
  engineType = static_cast<EngineType>(engineTypeByte);
}

void SimConnectAircraft::write(QDataStream& out) const
{
  out << static_cast<quint32>(flags);

  writeString(out, airplaneTitle);
  writeString(out, airplaneModel);
  writeString(out, airplaneReg);
  writeString(out, airplaneType);
  writeString(out, airplaneAirline);
  writeString(out, airplaneFlightnumber);

  out << position.getLonX() << position.getLatY() << position.getAltitude() << headingTrue << headingMag
      << groundSpeed << indicatedSpeed << verticalSpeed
      << indicatedAltitude << trueSpeed << machSpeed
      << trackMag << trackTrue << magVar << numberOfEngines
      << static_cast<quint8>(category) << static_cast<quint8>(engineType);

}

} // namespace sc
} // namespace fs
} // namespace atools
