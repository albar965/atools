/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef LITTLENAVCONNECT_NAVSERVER_H
#define LITTLENAVCONNECT_NAVSERVER_H

#include "fs/ns/navservercommon.h"

#include <QMutex>
#include <QTcpServer>

namespace atools {
namespace fs {
namespace sc {

class SimConnectData;
class DataReaderThread;
}

namespace ns {

class NavServerWorker;

/* Tcp server that will spawn a new thread with NavServerWorker for each connection.
 * Simulator data is send to each of these workers from DataReaderThread. */
class NavServer :
  public QTcpServer
{
  Q_OBJECT

public:
  NavServer(QObject *parent, atools::fs::ns::NavServerOptions optionFlags, int inetPort);
  virtual ~NavServer();

  bool startServer(atools::fs::sc::DataReaderThread *dataReaderThread);
  void stopServer();

  /* true if any workers are in the list */
  bool hasConnections() const;

  /* Need a stop/start to use new port */
  void setPort(int value)
  {
    port = value;
  }

private:
  void incomingConnection(qintptr socketDescriptor) override;
  void threadFinished(NavServerWorker *worker);

  atools::fs::ns::NavServerOptions options = NONE;
  atools::fs::sc::DataReaderThread *dataReader;

  QSet<NavServerWorker *> workers;
  // Needed to lock for any modifications of the workers set
  mutable QMutex threadsMutex;

  int port = 51968;
};

} // namespace ns
} // namespace fs
} // namespace atools

#endif // LITTLENAVCONNECT_NAVSERVER_H
