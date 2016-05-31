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

#ifndef ATOOLS_FS_SIMCONNECTDATA_H
#define ATOOLS_FS_SIMCONNECTDATA_H

#include "geo/pos.h"
#include "fs/sc/types.h"

#include <QString>

class QIODevice;

namespace atools {
namespace fs {
namespace sc {

// quint16
enum Flag
{
  NONE = 0x00,
  ON_GROUND = 0x01
              // IN_CLOUD = 0x02,
              // IN_RAIN = 0x04,
              // IN_SNOW = 0x08
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::Flags);

class SimConnectData
{
public:
  SimConnectData();
  SimConnectData(const SimConnectData& other);
  ~SimConnectData();

  bool read(QIODevice *ioDevice);

  int write(QIODevice *ioDevice);

  const QString& getAirplaneTitle() const
  {
    return airplaneTitle;
  }

  void setAirplaneTitle(const QString& value)
  {
    airplaneTitle = value;
  }

  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  void setAirplaneModel(const QString& value)
  {
    airplaneModel = value;
  }

  const QString& getAirplaneReg() const
  {
    return airplaneReg;
  }

  void setAirplaneReg(const QString& value)
  {
    airplaneReg = value;
  }

  int getPacketId() const
  {
    return static_cast<int>(packetId);
  }

  void setPacketId(int value)
  {
    packetId = static_cast<quint32>(value);
  }

  int getPacketTs() const
  {
    return static_cast<int>(packetTs);
  }

  void setPacketTs(unsigned int value)
  {
    packetTs = static_cast<quint32>(value);
  }

  atools::geo::Pos& getPosition()
  {
    return position;
  }

  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  void setPosition(const atools::geo::Pos& value)
  {
    position = value;
  }

  float getCourseTrue() const
  {
    return courseTrue;
  }

  void setCourseTrue(float value)
  {
    courseTrue = value;
  }

  float getCourseMag() const
  {
    return courseMag;
  }

  void setCourseMag(float value)
  {
    courseMag = value;
  }

  float getGroundSpeed() const
  {
    return groundSpeed;
  }

  void setGroundSpeed(float value)
  {
    groundSpeed = value;
  }

  float getIndicatedSpeed() const
  {
    return indicatedSpeed;
  }

  void setIndicatedSpeed(float value)
  {
    indicatedSpeed = value;
  }

  float getWindSpeed() const
  {
    return windSpeed;
  }

  void setWindSpeed(float value)
  {
    windSpeed = value;
  }

  float getWindDirection() const
  {
    return windDirection;
  }

  void setWindDirection(float value)
  {
    windDirection = value;
  }

  float getVerticalSpeed() const
  {
    return verticalSpeed;
  }

  void setVerticalSpeed(float value)
  {
    verticalSpeed = value;
  }

  const QString& getAirplaneType() const
  {
    return airplaneType;
  }

  void setAirplaneType(const QString& value)
  {
    airplaneType = value;
  }

  const QString& getAirplaneAirline() const
  {
    return airplaneAirline;
  }

  void setAirplaneAirline(const QString& value)
  {
    airplaneAirline = value;
  }

  const QString& getAirplaneFlightnumber() const
  {
    return airplaneFlightnumber;
  }

  void setAirplaneFlightnumber(const QString& value)
  {
    airplaneFlightnumber = value;
  }

  atools::fs::sc::SimConnectStatus getStatus() const
  {
    return status;
  }

  const QString& getStatusText() const
  {
    return SimConnectStatusText.at(status);
  }

  float getIndicatedAltitude() const
  {
    return indicatedAltitude;
  }

  void setIndicatedAltitude(float value)
  {
    indicatedAltitude = value;
  }

  float getAltitudeAboveGround() const
  {
    return altitudeAboveGround;
  }

  void setAltitudeAboveGround(float value)
  {
    altitudeAboveGround = value;
  }

  float getGroundAltitude() const
  {
    return groundAltitude;
  }

  void setGroundAltitude(float value)
  {
    groundAltitude = value;
  }

  float getTrueSpeed() const
  {
    return trueSpeed;
  }

  void setTrueSpeed(float value)
  {
    trueSpeed = value;
  }

  float getMachSpeed() const
  {
    return machSpeed;
  }

  void setMachSpeed(float value)
  {
    machSpeed = value;
  }

  Flags getFlags() const
  {
    return flags;
  }

  Flags& getFlags()
  {
    return flags;
  }

  void setFlags(Flags value)
  {
    flags = value;
  }

  float getTrackMag() const
  {
    return trackMag;
  }

  void setTrackMag(float value)
  {
    trackMag = value;
  }

  float getTrackTrue() const
  {
    return trackTrue;
  }

  void setTrackTrue(float value)
  {
    trackTrue = value;
  }

  float getAmbientTemperature() const
  {
    return ambientTemperature;
  }

  void setAmbientTemperature(float value)
  {
    ambientTemperature = value;
  }

  float getTotalAirTemperature() const
  {
    return totalAirTemperature;
  }

  void setTotalAirTemperature(float value)
  {
    totalAirTemperature = value;
  }

  float getSeaLevelPressure() const
  {
    return seaLevelPressure;
  }

  void setSeaLevelPressure(float value)
  {
    seaLevelPressure = value;
  }

  float getPitotIce() const
  {
    return pitotIce;
  }

  void setPitotIce(float value)
  {
    pitotIce = value;
  }

  float getStructuralIce() const
  {
    return structuralIce;
  }

  void setStructuralIce(float value)
  {
    structuralIce = value;
  }

  float getAirplaneTotalWeight() const
  {
    return airplaneTotalWeight;
  }

  void setAirplaneTotalWeight(float value)
  {
    airplaneTotalWeight = value;
  }

  float getAirplaneMaxGrossWeight() const
  {
    return airplaneMaxGrossWeight;
  }

  void setAirplaneMaxGrossWeight(float value)
  {
    airplaneMaxGrossWeight = value;
  }

  float getAirplaneEmptyWeight() const
  {
    return airplaneEmptyWeight;
  }

  void setAirplaneEmptyWeight(float value)
  {
    airplaneEmptyWeight = value;
  }

  float getFuelTotalQuantity() const
  {
    return fuelTotalQuantity;
  }

  void setFuelTotalQuantity(float value)
  {
    fuelTotalQuantity = value;
  }

  float getFuelTotalWeight() const
  {
    return fuelTotalWeight;
  }

  void setFuelTotalWeight(float value)
  {
    fuelTotalWeight = value;
  }

  float getFuelFlowPPH() const
  {
    return fuelFlowPPH;
  }

  void setFuelFlowPPH(float value)
  {
    fuelFlowPPH = value;
  }

  float getFuelFlowGPH() const
  {
    return fuelFlowGPH;
  }

  void setFuelFlowGPH(float value)
  {
    fuelFlowGPH = value;
  }

  static int getDataVersion()
  {
    return DATA_VERSION;
  }

  float getMagVar() const
  {
    return magVar;
  }

  void setMagVar(float value)
  {
    magVar = value;
  }

  unsigned int getLocalTime() const;
  void setLocalTime(int value);

  unsigned int getZuluTime() const;
  void setZuluTime(int value);

private:
  const static quint16 MAGIC_NUMBER_DATA = 0x5A5A;
  const static quint16 DATA_VERSION = 4;

  void writeString(QDataStream& out, const QString& str) const;
  bool readString(QDataStream& in, QString& str, quint16 *size = nullptr);

  QString airplaneTitle, airplaneModel, airplaneReg, airplaneType,
          airplaneAirline, airplaneFlightnumber;

  atools::geo::Pos position;
  float courseTrue = 0.f, courseMag = 0.f, groundSpeed = 0.f, indicatedAltitude = 0.f,
        altitudeAboveGround = 0.f, groundAltitude = 0.f, indicatedSpeed = 0.f, trueSpeed = 0.f,
        machSpeed = 0.f, windSpeed = 0.f, windDirection = 0.f, verticalSpeed = 0.f;

  // New since version 3
  float trackMag = 0.f, trackTrue = 0.f, ambientTemperature = 0.f, totalAirTemperature = 0.f,
        seaLevelPressure = 0.f, pitotIce = 0.f, structuralIce = 0.f, airplaneTotalWeight = 0.f,
        airplaneMaxGrossWeight = 0.f, airplaneEmptyWeight = 0.f, fuelTotalQuantity = 0.f,
        fuelTotalWeight = 0.f, fuelFlowPPH = 0.f, fuelFlowGPH = 0.f, magVar = 0.f;
  quint32 localTime = 0, zuluTime = 0;

  Flags flags = atools::fs::sc::NONE;
  quint32 packetId = 0, packetTs = 0;
  atools::fs::sc::SimConnectStatus status = OK;
  quint16 magicNumber = 0, packetSize = 0, version = 1, padding;
  quint32 padding2;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectData);

Q_DECLARE_TYPEINFO(atools::fs::sc::SimConnectData, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_FS_SIMCONNECTDATA_H
