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

#ifndef ATOOLS_NS_NAVSERVERTHREAD_H
#define ATOOLS_NS_NAVSERVERTHREAD_H

#include "fs/sc/simconnectdata.h"

#include "fs/sc/simconnectreply.h"
#include "fs/ns/navservercommon.h"

#include <QHostInfo>
#include <QSet>

class QTcpSocket;

namespace atools {
namespace fs {
namespace ns {

class NavServer;

/* Worker for threads that are spawned for each incoming connection. Worker approach is used to ensure that
 * singals to this object are using this thread's context. */
class NavServerWorker :
  public QObject
{
  Q_OBJECT

public:
  NavServerWorker(qintptr socketDescriptor, NavServer *parent, atools::fs::ns::NavServerOptions optionFlags);
  virtual ~NavServerWorker() override;

  /* Receives sim connect data from DataReader thread and writes to socket. */
  void postSimConnectData(atools::fs::sc::SimConnectData dataPacket);

  /* Signal posted by thread to indicate it has started . */
  void threadStarted();

signals:
  /* Weather received from socket. Set to data reader. */
  void postWeatherRequest(atools::fs::sc::WeatherRequest request);

private:
  /* Connection closed from remote end. */
  void socketDisconnected();

  /* Read reply from remote end. */
  void readyReadReplyFromSocket();

  /* Count dropped packages and write a message if too many accumulated. */
  void handleDroppedPackages(const QString& reason);

  const int MAX_DROPPED_PACKAGES = 50;

  qintptr socketDescr;
  atools::fs::sc::SimConnectData data;
  QTcpSocket *socket = nullptr;

  atools::fs::ns::NavServerOptions options = NONE;

  /* Count dropped packages to give a warning to the user */
  int droppedPackages = 0;
  bool inPost = false;

  /* Add packet id on send and remove when reply is received */
  QSet<int> lastPacketIds;
  QString peerAddr;
  QHostInfo hostInfo;

};

} // namespace ns
} // namespace fs
} // namespace atools

#endif // ATOOLS_NS_NAVSERVERTHREAD_H
