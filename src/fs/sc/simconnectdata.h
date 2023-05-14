/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
#include "fs/weather/weathertypes.h"
#include "fs/sc/simconnectuseraircraft.h"

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

/*
 * Class that transfers flight simulator data read using the simconnect interface across the network to
 * a client like Little Navmap. A version of the protocol is maintained to check for application compatibility.
 */
class SimConnectData :
  public SimConnectDataBase
{
public:
  SimConnectData();
  SimConnectData(const SimConnectData& other);
  virtual ~SimConnectData() override;

  /*
   * Read from IO device.
   * @return true if it was fully read. False if not or an error occured.
   */
  bool read(QIODevice *ioDevice);

  /*
   * Write to IO device.
   * @return number of bytes written
   */
  int write(QIODevice *ioDevice);

  // metadata ----------------------------------------------------
  /* Serial number for data packet. */
  int getPacketId() const
  {
    return static_cast<int>(packetId);
  }

  /* Sequence number used to match request and reply. For weather always 0 */
  void setPacketId(int value)
  {
    packetId = static_cast<quint32>(value);
  }

  /* Packet creating timestamp in seconds since epoch */
  QDateTime getPacketTimestamp() const
  {
    return packetTs;
  }

  void setPacketTimestamp(QDateTime value)
  {
    packetTs = value;
  }

  /*
   * @return data version for this packet format
   */
  static int getDataVersion()
  {
    return DATA_VERSION;
  }

  // fs data ----------------------------------------------------

  const atools::fs::sc::SimConnectUserAircraft& getUserAircraftConst() const
  {
    return userAircraft;
  }

  const QVector<atools::fs::sc::SimConnectAircraft>& getAiAircraftConst() const
  {
    return aiAircraft;
  }

  atools::fs::sc::SimConnectUserAircraft& getUserAircraft()
  {
    return userAircraft;
  }

  QVector<atools::fs::sc::SimConnectAircraft>& getAiAircraft()
  {
    return aiAircraft;
  }

  void clearAiAircraft()
  {
    aiAircraft.clear();
    aiAircraftIndex.clear();
  }

  /* Get aircraft by object ID */
  atools::fs::sc::SimConnectAircraft *getAiAircraftById(int id);
  const atools::fs::sc::SimConnectAircraft *getAiAircraftConstById(int id) const;

  const QVector<atools::fs::weather::MetarResult>& getMetars() const
  {
    return metarResults;
  }

  void setMetars(const QVector<atools::fs::weather::MetarResult>& value)
  {
    metarResults = value;
  }

  /* Empty weather replies or metar replys. Aircraft is not valid. */
  bool isEmptyReply() const
  {
    return packetId == 0;
  }

  static SimConnectData buildDebugForPosition(const atools::geo::Pos& pos, const atools::geo::Pos& lastPos, bool ground,
                                              float vertSpeed, float tas, float fuelflow, float totalFuel, float ice, float flightplanAlt,
                                              float magVar, bool jetFuel, bool helicopter);

  bool isUserAircraftValid() const
  {
    return userAircraft.position.isValid();
  }

  /* Update short registration keys which can be used for cross referencing */
  void updateIndexesAndKeys();

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class xpc::XpConnect;

  const static quint32 MAGIC_NUMBER_DATA = 0xF75E0AF3;
  const static quint32 DATA_VERSION = 11;

  quint32 packetId = 0;
  QDateTime packetTs;
  quint32 magicNumber = 0, packetSize = 0, version = 0;

  atools::fs::sc::SimConnectUserAircraft userAircraft;
  QVector<atools::fs::sc::SimConnectAircraft> aiAircraft;

  // Maps objectId to index in vector aiAircraft - not transferred
  QHash<int, int> aiAircraftIndex;

  QVector<atools::fs::weather::MetarResult> metarResults;
};

const static atools::fs::sc::SimConnectData EMPTY_SIMCONNECT_DATA;
const static atools::fs::sc::SimConnectAircraft EMPTY_SIMCONNECT_AIRCRAFT;

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectData);

#endif // ATOOLS_FS_SIMCONNECTDATA_H
