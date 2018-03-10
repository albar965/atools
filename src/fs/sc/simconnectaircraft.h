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

#ifndef ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H
#define ATOOLS_FS_SC_SIMCONNECTAIRPLANE_H

#include "geo/pos.h"
#include "fs/sc/simconnectdatabase.h"

#include <QString>
#include <QDateTime>

class QIODevice;

namespace xpc {
class XpConnect;
}

namespace atools {
namespace fs {
namespace sc {

class SimConnectHandler;
class SimConnectHandlerPrivate;
class SimConnectData;

// quint8
enum Category
{
  AIRPLANE,
  HELICOPTER,
  BOAT,
  GROUNDVEHICLE,
  CONTROLTOWER,
  SIMPLEOBJECT,
  VIEWER,
  UNKNOWN
};

// quint8
enum EngineType
{
  PISTON = 0,
  JET = 1,
  NO_ENGINE = 2,
  HELO_TURBINE = 3,
  UNSUPPORTED = 4,
  TURBOPROP = 5
};

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

  /* Mooney, Boeing, */
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

  /* Beech Baron 58 Paint 1 */
  const QString& getAirplaneTitle() const
  {
    return airplaneTitle;
  }

  /* Short ICAO code MD80, BE58, etc. */
  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  /* N71FS */
  const QString& getAirplaneRegistration() const
  {
    return airplaneReg;
  }

  /* Includes actual altitude in feet */
  atools::geo::Pos& getPosition()
  {
    return position;
  }

  /* Includes actual altitude in feet */
  const atools::geo::Pos& getPosition() const
  {
    return position;
  }

  float getHeadingDegTrue() const
  {
    return headingTrueDeg;
  }

  float getHeadingDegMag() const
  {
    return headingMagDeg;
  }

  float getGroundSpeedKts() const
  {
    return groundSpeedKts;
  }

  float getIndicatedSpeedKts() const
  {
    return indicatedSpeedKts;
  }

  float getVerticalSpeedFeetPerMin() const
  {
    return verticalSpeedFeetPerMin;
  }

  float getIndicatedAltitudeFt() const
  {
    return indicatedAltitudeFt;
  }

  float getTrueSpeedKts() const
  {
    return trueSpeedKts;
  }

  float getMachSpeed() const
  {
    return machSpeed;
  }

  AircraftFlags getFlags() const
  {
    return flags;
  }

  AircraftFlags& getFlags()
  {
    return flags;
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

  int getId()
  {
    return static_cast<int>(objectId);
  }

  int getModelRadius() const
  {
    return modelRadiusFt;
  }

  /* Use an estimate based on engine type in ft - used for map display and not for HTML info */
  int getModelRadiusCorrected() const;

  int getWingSpan() const
  {
    return wingSpanFt;
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

  bool isSimPaused() const
  {
    return flags & SIM_PAUSED;
  }

  bool isSimReplay() const
  {
    return flags & SIM_REPLAY;
  }

  /* Create artificially by mouse movements */
  bool isDebug() const
  {
    return debug;
  }

  /* Compares only registration, type and others */
  bool isSameAircraft(const SimConnectAircraft& other) const;

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectHandlerPrivate;
  friend class atools::fs::sc::SimConnectData;
  friend class xpc::XpConnect;

  QString airplaneTitle, airplaneType, airplaneModel, airplaneReg,
          airplaneAirline, airplaneFlightnumber, fromIdent, toIdent;

  atools::geo::Pos position;
  float headingTrueDeg = 0.f, headingMagDeg = 0.f, groundSpeedKts = 0.f, indicatedAltitudeFt = 0.f,
        indicatedSpeedKts = 0.f, trueSpeedKts = 0.f,
        machSpeed = 0.f, verticalSpeedFeetPerMin = 0.f;
  quint16 modelRadiusFt = 0, wingSpanFt = 0;

  quint32 objectId = 0L;

  AircraftFlags flags = atools::fs::sc::NONE;

  Category category;
  EngineType engineType = atools::fs::sc::UNSUPPORTED;
  quint8 numberOfEngines = 0;
  bool debug = false;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectAircraft);

#endif // ATOOLS_FS_SIMCONNECTAIRPLANE_H
