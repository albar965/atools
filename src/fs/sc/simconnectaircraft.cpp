/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/sc/simconnectaircraft.h"
#include "atools.h"

#include <QDebug>
#include <QDataStream>
#include <QRegularExpression>

namespace atools {
namespace fs {
namespace sc {

SimConnectAircraft::SimConnectAircraft()
{

}

SimConnectAircraft::~SimConnectAircraft()
{

}

void SimConnectAircraft::read(QDataStream& in)
{
  in >> objectId;

  quint8 byteFlags;
  in >> byteFlags;
  dataFlags = DataFlags(byteFlags);

  quint16 shortFlags;
  in >> shortFlags;
  flags = AircraftFlags(shortFlags);

  if(!(dataFlags & DATA_STRINGS_OMITTED))
  {
    readString(in, airplaneTitle);
    readString(in, airplaneModel);
    readString(in, airplaneReg);
    readString(in, airplaneType);
    readString(in, airplaneAirline);
    readString(in, airplaneFlightnumber);
    readString(in, fromIdent);
    readString(in, toIdent);
  }

  float lonx, laty, altitude;
  quint8 categoryByte, engineTypeByte;

  in >> lonx >> laty >> altitude >> headingTrueDeg >> headingMagDeg >> groundSpeedKts >> indicatedSpeedKts
  >> verticalSpeedFeetPerMin >> indicatedAltitudeFt >> trueAirspeedKts >> machSpeed >> numberOfEngines
  >> wingSpanFt >> modelRadiusFt >> deckHeight >> categoryByte >> engineTypeByte >> transponderCode >> properties;

  position.setAltitude(altitude);
  position.setLonX(lonx);
  position.setLatY(laty);

  category = static_cast<Category>(categoryByte);
  engineType = static_cast<EngineType>(engineTypeByte);
}

int SimConnectAircraft::getModelRadiusCorrected() const
{
  if(modelRadiusFt > 0)
    return modelRadiusFt;
  else
  {
    if(category == CARRIER)
      return 500;
    else if(category == FRIGATE)
      return 160;
    else
    {
      switch(engineType)
      {
        case PISTON:
        case NO_ENGINE:
        case HELO_TURBINE:
        case UNSUPPORTED:
          return 20;

        case JET:
          return 60;

        case TURBOPROP:
          return 40;
      }
    }
    return 20;
  }
}

void SimConnectAircraft::write(QDataStream& out) const
{
  out << objectId << static_cast<quint8>(dataFlags) << static_cast<quint16>(flags);

  if(!(dataFlags & DATA_STRINGS_OMITTED))
  {
    writeString(out, airplaneTitle);
    writeString(out, airplaneModel);
    writeString(out, airplaneReg);
    writeString(out, airplaneType);
    writeString(out, airplaneAirline);
    writeString(out, airplaneFlightnumber);
    writeString(out, fromIdent);
    writeString(out, toIdent);
  }

  out << position.getLonX() << position.getLatY() << position.getAltitude() << headingTrueDeg << headingMagDeg
      << groundSpeedKts << indicatedSpeedKts << verticalSpeedFeetPerMin
      << indicatedAltitudeFt << trueAirspeedKts << machSpeed
      << numberOfEngines << wingSpanFt << modelRadiusFt << deckHeight
      << static_cast<quint8>(category) << static_cast<quint8>(engineType) << transponderCode << properties;
}

atools::geo::PosD SimConnectAircraft::getPositionD() const
{
  if(properties.contains(PROP_AIRCRAFT_LONX) && properties.contains(PROP_AIRCRAFT_LATY))
    return atools::geo::PosD(properties.value(PROP_AIRCRAFT_LONX).getValueDouble(),
                             properties.value(PROP_AIRCRAFT_LATY).getValueDouble(), position.getAltitude());
  else
    return atools::geo::PosD(position);
}

void SimConnectAircraft::getXPlaneDate(int& localDateDays, float& localTimeSec, float& zuluTimeSec)
{
  if(properties.contains(PROP_LOCAL_DATE_DAYS) && properties.contains(PROP_LOCAL_TIME_SEC) && properties.contains(PROP_ZULU_TIME_SEC))
  {
    localDateDays = properties.value(PROP_LOCAL_DATE_DAYS).getValueInt();
    localTimeSec = properties.value(PROP_LOCAL_TIME_SEC).getValueFloat();
    zuluTimeSec = properties.value(PROP_ZULU_TIME_SEC).getValueFloat();
  }
  else
  {
    localDateDays = SC_INVALID_INT;
    localTimeSec = zuluTimeSec = SC_INVALID_FLOAT;
  }
}

int SimConnectAircraft::getId() const
{
  if(objectId > std::numeric_limits<int>::max())
    qWarning() << Q_FUNC_INFO << "Object id" << objectId << "exceeds maximum";

  return static_cast<int>(objectId);
}

bool SimConnectAircraft::isSameAircraft(const SimConnectAircraft& other) const
{
  return airplaneTitle == other.airplaneTitle &&
         airplaneModel == other.airplaneModel &&
         airplaneReg == other.airplaneReg &&
         airplaneType == other.airplaneType &&
         airplaneAirline == other.airplaneAirline &&
         airplaneFlightnumber == other.airplaneFlightnumber;
}

void SimConnectAircraft::updateAircraftNames(const QString& airplaneTypeParam, const QString& airplaneAirlineParam,
                                             const QString& airplaneTitleParam, const QString& airplaneModelParam)
{
  airplaneType = airplaneTypeParam;
  airplaneAirline = airplaneAirlineParam;
  airplaneTitle = airplaneTitleParam;
  airplaneModel = airplaneModelParam;
}

bool SimConnectAircraft::nameValid(const QString& name) const
{
  // "ATCCOM.ATC_NAME CESSNA.0.text" or "$$:Generic"
  return !name.startsWith("$$") && !name.startsWith("ATCCOM");
}

void SimConnectAircraft::cleanAircraftNames()
{
  const static QRegularExpression ATC_NAME_REGEXP("ATCCOM\\.AT?C[_ ]+NAME[_ ]+([A-Z0-9]+)\\.0\\.(text|txt)",
                                                  QRegularExpression::CaseInsensitiveOption);
  const static QRegularExpression ATC_MODEL_REGEXP("ATCCOM\\.AT?C[_ ]+MODEL[_ ]+([A-Z0-9]+)\\.0\\.(text|txt)",
                                                   QRegularExpression::CaseInsensitiveOption);
  // MSFS 2024
  // aircraft.airplaneTitle "C172SP G1000 Passengers"
  // aircraft.airplaneModel "ATCCOM.AC_MODEL C172.0.text"
  // aircraft.airplaneReg "D-FKHU"
  // aircraft.airplaneType "ATCCOM.ATC_NAME CESSNA.0.text"
  // aircraft.airplaneAirline ""
  // aircraft.airplaneFlightnumber ""

  QString type = atools::capString(ATC_NAME_REGEXP.match(airplaneType).captured(1));
  if(!type.isEmpty())
    airplaneType = type;

  if(!nameValid(airplaneType))
    airplaneType.clear();

  if(!nameValid(airplaneAirline))
    airplaneAirline.clear();

  if(!nameValid(airplaneTitle))
    airplaneTitle.clear();

  QString model = ATC_MODEL_REGEXP.match(airplaneModel).captured(1);
  if(!model.isEmpty())
    airplaneModel = model;

  if(!nameValid(airplaneModel))
    airplaneModel.clear();
}

QString SimConnectAircraft::getTransponderCodeStr() const
{
  // Get number as octal
  return transponderCode != -1 ? QStringLiteral("%1").arg(transponderCode, 4, 8, QChar('0')) : QString();
}

} // namespace sc
} // namespace fs
} // namespace atools
