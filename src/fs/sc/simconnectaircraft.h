/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
#include "util/props.h"

#include <QString>
#include <cmath>

class QIODevice;

namespace xpc {
class XpConnect;
class AircraftFileLoader;
}

namespace atools {
namespace fs {
namespace online {
class OnlinedataManager;
}

namespace sc {

class SimConnectHandler;
class SimConnectHandlerPrivate;
class SimConnectData;

enum Category : quint8
{
  AIRPLANE,
  HELICOPTER,
  BOAT,
  CARRIER,
  FRIGATE,
  GROUNDVEHICLE,
  CONTROLTOWER,
  SIMPLEOBJECT,
  VIEWER,
  UNKNOWN
};

enum EngineType : quint8
{
  PISTON = 0,
  JET = 1,
  NO_ENGINE = 2,
  HELO_TURBINE = 3,
  UNSUPPORTED = 4,
  TURBOPROP = 5
};

enum DataFlags : quint8
{
  NO_FLAGS = 0,
  DATA_STRINGS_OMITTED = 1 << 0 // Strings are omitted for partial transfer
                         // and are only transferred every tenth packets - currenty not used
};

enum SimConnectAircraftPropTypes
{
  PROP_AIRCRAFT_CFG,
  PROP_AIRCRAFT_LONX,
  PROP_AIRCRAFT_LATY,
  PROP_XPCONNECT_VERSION
};

/*
 * Base aircraft that is used to transfer across network links. For user and AI aircraft.
 */
class SimConnectAircraft :
  public SimConnectDataBase
{
public:
  SimConnectAircraft();
  virtual ~SimConnectAircraft() override;

  virtual void read(QDataStream& in);
  virtual void write(QDataStream& out) const;

  // fs data ----------------------------------------------------

  /* Mooney, Boeing, Actually aircraft model. */
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

  /* ICAO aircraft designator: MD80, BE58, etc. */
  const QString& getAirplaneModel() const
  {
    return airplaneModel;
  }

  /* Update after loading from aircraft.cfg */
  void setAirplaneModel(const QString& value)
  {
    airplaneModel = value;
  }

  /* Update after loading from aircraft.cfg */
  void setCategory(Category value)
  {
    category = value;
  }

  /* N71FS */
  const QString& getAirplaneRegistration() const
  {
    return airplaneReg;
  }

  /* "dedfs" for "D-EDFS" */
  const QString& getAirplaneRegistrationKey() const
  {
    return airplaneRegKey;
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

  /* More accurate optional position as double */
  atools::geo::PosD getPositionD() const
  {
    if(properties.contains(PROP_AIRCRAFT_LONX) && properties.contains(PROP_AIRCRAFT_LATY))
      return atools::geo::PosD(properties.value(PROP_AIRCRAFT_LONX).getValueDouble(),
                               properties.value(PROP_AIRCRAFT_LATY).getValueDouble(), position.getAltitude());
    else
      return atools::geo::PosD(position);
  }

  /* Leaves altitude intact */
  void setCoordinates(atools::geo::Pos value)
  {
    value.setAltitude(position.getAltitude());
    position = value;
  }

  void setActualAltitude(float altFt)
  {
    position.setAltitude(altFt);
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

  /* Altitude as shown in the aircraft instruments. Depends on baro setting. */
  float getIndicatedAltitudeFt() const
  {
    return indicatedAltitudeFt;
  }

  /* Real altitude in ft independent of baro setting */
  float getActualAltitudeFt() const
  {
    return position.getAltitude();
  }

  float getTrueAirspeedKts() const
  {
    return trueAirspeedKts;
  }

  float getMachSpeed() const
  {
    return machSpeed;
  }

  AircraftFlags getFlags() const
  {
    return flags;
  }

  void setFlags(AircraftFlags value)
  {
    flags = value;
  }

  void setFlag(AircraftFlags value, bool on = true)
  {
    if(on)
      flags |= value;
    else
      flags &= ~value;
  }

  atools::fs::sc::Category getCategory() const
  {
    return category;
  }

  bool isAnyBoat() const
  {
    return category == BOAT || category == CARRIER || category == FRIGATE;
  }

  bool isAnyFlying() const
  {
    return category == AIRPLANE || category == HELICOPTER;
  }

  bool isHelicopter() const
  {
    return category == HELICOPTER || engineType == HELO_TURBINE;
  }

  EngineType getEngineType() const
  {
    return engineType;
  }

  int getNumberOfEngines() const
  {
    return numberOfEngines;
  }

  /* modeS_id for X-Plane: integer 24bit (0-16777215 or 0 - 0xFFFFFF) unique ID of the airframe. This is also known as the ADS-B "hexcode".
   * dwObjectID for SimConnect
   * Table and column "client.client_id" for online aircraft. Calculated as semi-permanent id. */
  unsigned int getObjectId() const
  {
    return objectId;
  }

  /* ID used for database objects */
  int getId() const;

  int getModelRadius() const
  {
    return modelRadiusFt;
  }

  /* Use an estimate based on engine type in ft - used for map display and not for HTML info */
  int getModelRadiusCorrected() const;

  int getModelSize() const
  {
    return getWingSpan() > 0 ? getWingSpan() : getModelRadiusCorrected() * 2;
  }

  int getWingSpan() const
  {
    return wingSpanFt;
  }

  int getDeckHeight() const
  {
    return deckHeight;
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

  bool isOnline() const
  {
    return flags & SIM_ONLINE;
  }

  /* This simulator aircraft is a shadow of an online network aircraft (by matching callsign to reg) */
  bool isOnlineShadow() const
  {
    return flags & SIM_ONLINE_SHADOW;
  }

  /* Create artificially by mouse movements */
  bool isDebug() const
  {
    return debug;
  }

  /* true if not empty default initialized object */
  bool isValid() const
  {
    return position.isValid();
  }

  /* true if not empty default initialized object and not still at pos 0/0.
   * Also check for broken altitude values as delivered by MSFS 2024. */
  bool isFullyValid() const
  {
    return isValid() && !(groundSpeedKts < 5.0f && position.almostEqual(atools::geo::Pos(0.f, 0.f), 0.1f)) && isActualAltitudeFullyValid();
  }

  bool isActualAltitudeFullyValid() const
  {
    return !std::isnan(position.getAltitude()) && position.getAltitude() > -2000.f && position.getAltitude() < 200000.f;
  }

  /* Compares only registration, type and others */
  bool isSameAircraft(const SimConnectAircraft& other) const;

  /* Update all names which allows to replace translation key in MSFS with local translated aircraft names */
  void updateAircraftNames(const QString& airplaneTypeParam, const QString& airplaneAirlineParam,
                           const QString& airplaneTitleParam, const QString& airplaneModelParam);

  /* Clear all names which are translation of other keys */
  void cleanAircraftNames();

  const atools::util::Props& getProperties() const
  {
    return properties;
  }

  void setProperties(const atools::util::Props& value)
  {
    properties = value;
  }

  /* 4095 = 07777 = 0xFFF = 0b111111111111,
   * -1 = invalid / not available */
  int getTransponderCode() const
  {
    return transponderCode;
  }

  bool isTransponderCodeValid() const
  {
    return transponderCode != -1;
  }

  /* Returns "7777" for 4095/07777/0xFFF/0b111111111111
   *  Empty string if not available */
  QString getTransponderCodeStr() const
  {
    // Get number as octal
    return transponderCode != -1 ? QString("%1").arg(transponderCode, 4, 8, QChar('0')) : QString();
  }

  DataFlags getDataFlags() const
  {
    return dataFlags;
  }

  void setDataFlags(DataFlags value)
  {
    dataFlags = value;
  }

  /* Returns true if user changed aircraft */
  bool hasAircraftChanged(const SimConnectAircraft& other)
  {
    return getAirplaneTitle() != other.getAirplaneTitle() ||
           getAirplaneType() != other.getAirplaneType() ||
           getAirplaneModel() != other.getAirplaneModel() ||
           getAirplaneRegistration() != other.getAirplaneRegistration() ||
           getAirplaneAirline() != other.getAirplaneAirline() ||
           getAirplaneFlightnumber() != other.getAirplaneFlightnumber();
  }

  /* For debugging purposes */
  void setAirplaneRegistration(const QString& value)
  {
    airplaneReg = value;
  }

  /* "dedfs" for "D-EDFS" not transferred in stream. Needs to be updated after loading in client.
   *  Used to match aircraft from sim/online. */
  void updateAirplaneRegistrationKey()
  {
    airplaneRegKey = airplaneRegistrationToKey(airplaneReg);
  }

  /* Converts "D-EDFS" to "dedfs" */
  static QString airplaneRegistrationToKey(QString registration)
  {
    return registration.remove(QChar('_')).remove(QChar('-')).remove(QChar(' ')).toLower();
  }

private:
  bool nameValid(const QString& name) const;

  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectHandlerPrivate;
  friend class atools::fs::sc::SimConnectData;
  friend class xpc::XpConnect;
  friend class xpc::AircraftFileLoader;
  friend class atools::fs::online::OnlinedataManager;

  QString airplaneTitle, airplaneType, airplaneModel, airplaneReg, airplaneRegKey,
          airplaneAirline, airplaneFlightnumber, fromIdent, toIdent;

  // Contains real altitude in ft independent of baro setting
  atools::geo::Pos position;

  float headingTrueDeg = 0.f, headingMagDeg = 0.f, groundSpeedKts = 0.f, indicatedAltitudeFt = 0.f,
        indicatedSpeedKts = 0.f, trueAirspeedKts = 0.f,
        machSpeed = 0.f, verticalSpeedFeetPerMin = 0.f;
  quint16 modelRadiusFt = 0, wingSpanFt = 0, deckHeight = 0;

  qint16 transponderCode = -1; // TCAS number - convert to octal system to get real number, 0o7777 = 4095
  // -1 = not available

  quint32 objectId = 0L;

  DataFlags dataFlags = atools::fs::sc::NO_FLAGS;
  AircraftFlags flags = atools::fs::sc::NONE;

  Category category = atools::fs::sc::UNKNOWN;
  EngineType engineType = atools::fs::sc::UNSUPPORTED;
  quint8 numberOfEngines = 0;
  bool debug = false;

  atools::util::Props properties;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectAircraft)

#endif // ATOOLS_FS_SIMCONNECTAIRPLANE_H
