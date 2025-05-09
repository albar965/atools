/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_SIMCONNECTREPLY_H
#define ATOOLS_FS_SIMCONNECTREPLY_H

#include "fs/sc/simconnecttypes.h"
#include "fs/sc/simconnectdatabase.h"
#include "fs/sc/weatherrequest.h"
#include "util/flags.h"

#include <QString>

class QIODevice;

namespace atools {
namespace fs {
namespace sc {

// quint16
enum Command : quint32
{
  CMD_NONE,
  CMD_WEATHER_REQUEST
};

ATOOLS_DECLARE_FLAGS_32(Commands, atools::fs::sc::Command)

/*
 * Class that contains replay data from a client for SimConnectData.
 * A version of the protocol is maintained to check for application compatibility.
 */
class SimConnectReply
  : public SimConnectDataBase
{
public:
  SimConnectReply();
  virtual ~SimConnectReply() override;

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

  /* Sequence number used to match request and reply. For weather always 0 */
  int getPacketId() const
  {
    return static_cast<int>(packetId);
  }

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
   * @return Error status for last reading or writing call
   */
  atools::fs::sc::SimConnectStatus getStatus() const
  {
    return status;
  }

  /*
   * @return data version for this packet format
   */
  static int getReplyVersion()
  {
    return REPLY_VERSION;
  }

  Commands getCommand() const
  {
    return command;
  }

  void setCommand(const Commands& value)
  {
    command = value;
  }

  const atools::fs::sc::WeatherRequest& getWeatherRequest()
  {
    return weatherRequest;
  }

  void setWeatherRequest(const atools::fs::sc::WeatherRequest& value)
  {
    weatherRequest = value;
  }

private:
  const static quint32 MAGIC_NUMBER_REPLY = 0x33ED8272;
  const static quint32 REPLY_VERSION = 5;

  quint32 packetId = 0;
  QDateTime packetTs;
  atools::fs::sc::SimConnectStatus replyStatus = OK;
  quint32 magicNumber = 0, packetSize = 0, version = 2;
  Commands command;
  atools::fs::sc::WeatherRequest weatherRequest;

};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectReply)

#endif // ATOOLS_FS_SIMCONNECTREPLY_H
