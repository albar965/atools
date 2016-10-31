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

#ifndef ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H
#define ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H

#include "geo/pos.h"
#include "fs/sc/simconnectdatabase.h"

#include <QString>
#include <QDateTime>

class QIODevice;

namespace atools {
namespace fs {
namespace sc {

class SimConnectHandler;
class SimConnectHandlerPrivate;

// quint16
enum Flag
{
  NONE = 0x0000,
  ON_GROUND = 0x0001,
  IN_CLOUD = 0x0002,
  IN_RAIN = 0x0004,
  IN_SNOW = 0x0008,
  IS_USER = 0x0010
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::Flags);

// quint8
enum CategoryEnum
{
  AIRPLANE,
  HELICOPTER,
  BOAT,
  GROUNDVEHICLE,
  CONTROLTOWER,
  SIMPLEOBJECT,
  VIEWER
};

Q_DECLARE_FLAGS(Category, CategoryEnum);

// quint8
enum EngineTypeEnum
{
  PISTON = 0,
  JET = 1,
  NO_ENGINE = 2,
  HELO_TURBINE = 3,
  UNSUPPORTED = 4,
  TURBOPROP = 5
};

Q_DECLARE_FLAGS(EngineType, EngineTypeEnum);

/*
 * Base aircraft that is used to transfer across network links. For user and AI aircraft.
 */
class SimConnectAircraft :
  public SimConnectDataBase
{
public:
  SimConnectAircraft();
  SimConnectAircraft(const SimConnectAircraft& other);
  virtual ~SimConnectAircraft();

  virtual void read(QDataStream& in);
  virtual void write(QDataStream& out) const;

  // fs data ----------------------------------------------------

  const QString& getAirplaneTitle() const
  {
    return airplaneTitle;
  }

  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  const QString& getAirplaneRegistration() const
  {
    return airplaneReg;
  }

  atools::geo::Pos& getPosition()
  {
    return position;
  }

  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  float getHeadingDegTrue() const
  {
    return headingTrue;
  }

  float getHeadingDegMag() const
  {
    return headingMag;
  }

  float getGroundSpeedKts() const
  {
    return groundSpeed;
  }

  float getIndicatedSpeedKts() const
  {
    return indicatedSpeed;
  }

  float getVerticalSpeedFeetPerMin() const
  {
    return verticalSpeed;
  }

  const QString& getAirplaneType() const
  {
    return airplaneType;
  }

  const QString& getAirplaneAirline() const
  {
    return airplaneAirline;
  }

  const QString& getAirplaneFlightnumber() const
  {
    return airplaneFlightnumber;
  }

  float getIndicatedAltitudeFt() const
  {
    return indicatedAltitude;
  }

  float getTrueSpeedKts() const
  {
    return trueSpeed;
  }

  float getMachSpeed() const
  {
    return machSpeed;
  }

  Flags getFlags() const
  {
    return flags;
  }

  Flags& getFlags()
  {
    return flags;
  }

  float getMagVarDeg() const
  {
    return magVar;
  }

  Category getCategory() const
  {
    return category;
  }

  EngineType getEngineType() const
  {
    return engineType;
  }

  int getNumberOfEngines() const
  {
    return numberOfEngines;
  }

  unsigned int getObjectId() const
  {
    return objectId;
  }

  int getModelRadius() const
  {
    return modelRadius;
  }

  int getWingSpan() const
  {
    return wingSpan;
  }

  const QString& getFromIdent() const
  {
    return fromIdent;
  }

  const QString& getToIdent() const
  {
    return toIdent;
  }

  bool isOnGround() const
  {
    return flags & ON_GROUND;
  }

  bool isUser() const
  {
    return flags & IS_USER;
  }

  bool inCloud() const
  {
    return flags & IN_CLOUD;
  }

  bool inRain() const
  {
    return flags & IN_RAIN;
  }

  bool inSnow() const
  {
    return flags & IN_SNOW;
  }

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectHandlerPrivate;

  QString airplaneTitle, airplaneType, airplaneModel, airplaneReg,
          airplaneAirline, airplaneFlightnumber, fromIdent, toIdent;

  atools::geo::Pos position;
  float headingTrue = 0.f, headingMag = 0.f, groundSpeed = 0.f, indicatedAltitude = 0.f,
        indicatedSpeed = 0.f, trueSpeed = 0.f,
        machSpeed = 0.f, verticalSpeed = 0.f;
  quint16 modelRadius = 0, wingSpan = 0;

  float magVar = 0.f;
  quint32 objectId = 0L;

  Flags flags = atools::fs::sc::NONE;

  Category category;
  EngineType engineType;
  quint8 numberOfEngines;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectAircraft);

Q_DECLARE_TYPEINFO(atools::fs::sc::SimConnectAircraft, Q_MOVABLE_TYPE);

#endif // ATOOLS_FS_SIMCONNECTAIRPLANE_H
