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
#include "fs/sc/simconnecttypes.h"
#include "fs/sc/simconnectuseraircraft.h"

#include <QString>
#include <QDateTime>

class QIODevice;

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
  virtual ~SimConnectData();

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

  void setPacketId(int value)
  {
    packetId = static_cast<quint32>(value);
  }

  /* Packet creating timestamp in seconds since epoch */
  int getPacketTimestamp() const
  {
    return static_cast<int>(packetTs);
  }

  void setPacketTimestamp(unsigned int value)
  {
    packetTs = static_cast<quint32>(value);
  }

  /*
   * @return data version for this packet format
   */
  static int getDataVersion()
  {
    return DATA_VERSION;
  }

  // fs data ----------------------------------------------------

  const atools::fs::sc::SimConnectUserAircraft& getUserAircraft() const
  {
    return userAircraft;
  }

  const QVector<atools::fs::sc::SimConnectAircraft>& getAiAircraft() const
  {
    return aiAircraft;
  }

private:
  friend class atools::fs::sc::SimConnectHandler;

  const static quint32 MAGIC_NUMBER_DATA = 0xF75E0AF3;
  const static quint32 DATA_VERSION = 7;

  quint32 packetId = 0, packetTs = 0;
  quint32 magicNumber = 0, packetSize = 0, version = 1;

  atools::fs::sc::SimConnectUserAircraft userAircraft;
  QVector<atools::fs::sc::SimConnectAircraft> aiAircraft;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectData);

Q_DECLARE_TYPEINFO(atools::fs::sc::SimConnectData, Q_MOVABLE_TYPE);

#endif // ATOOLS_FS_SIMCONNECTDATA_H
