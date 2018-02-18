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

#include "fs/ns/navserver.h"

#include "fs/ns/navservercommon.h"
#include "fs/ns/navserverworker.h"
#include "fs/sc/datareaderthread.h"
#include "settings/settings.h"

#include <QNetworkInterface>
#include <QHostInfo>

namespace atools {
namespace fs {
namespace ns {

const QString BLUESPAN("<span style=\"color: #0000ff; font-weight:bold\">"), ENDSPAN("</span>");

NavServer::NavServer(QObject *parent, atools::fs::ns::NavServerOptions optionFlags, int inetPort)
  : QTcpServer(parent), options(optionFlags), port(inetPort)
{
  qDebug("NavServer created");
}

NavServer::~NavServer()
{
  stopServer();
}

void NavServer::stopServer()
{
  qDebug() << "Navserver stopping";

  // Close tcp server to avoid accepting connections
  if(isListening())
    close();

  // Stop all worker threads
  QSet<NavServerWorker *> workersCopy(workers);
  for(NavServerWorker *worker : workersCopy)
  {
    worker->thread()->exit();
    worker->thread()->wait();
  }

  qDebug() << "NavServer deleted";
}

bool NavServer::startServer(atools::fs::sc::DataReaderThread *dataReaderThread)
{
  dataReader = dataReaderThread;
  qDebug() << "Navserver starting";

  QStringList hostNameList, hostIpList;
  bool retval = listen(QHostAddress::AnyIPv4, static_cast<quint16>(port));

  qDebug() << "localHostName" << QHostInfo::localHostName()
           << "localDomainName" << QHostInfo::localDomainName();

  // Collect hostnames and IPs from all interfaces
  QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
  for(const QHostAddress& ip : ipAddressesList)
  {
    if(!ip.isLoopback() && !ip.isNull() && ip.protocol() == QAbstractSocket::IPv4Protocol)
    {
      QString name = QHostInfo::fromName(ip.toString()).hostName();
      qDebug() << "Found valid IP" << ip.toString() << "name" << name;
      if(!name.isEmpty() && name != ip.toString())
        hostNameList.append(name);
      hostIpList.append(ip.toString());
    }
    else
      qDebug() << "Found IP" << ip.toString();
  }

  // Add localhost if nothing was found
  if(hostNameList.isEmpty())
    hostNameList.append("localhost");

  if(hostIpList.isEmpty())
    hostIpList.append(QHostAddress(QHostAddress::LocalHost).toString());

  qDebug() << "Server address IP" << serverAddress();

  if(!retval)
    qCritical(gui).noquote().nospace() << tr("Unable to start the server: %1.").arg(errorString());
  else
  {
    if(options & HIDE_HOST)
      qInfo(gui).noquote().nospace() << tr("Server is listening.");
    else if(options & NO_HTML)
    {
      QString hostname = hostNameList.size() > 1 ? tr("Server is listening on hostnames %1 ") :
                         tr("Server is listening on hostname %1 ");

      QString ipaddr = hostIpList.size() > 1 ? tr("(IP addresses %2) ") : tr("(IP address %2) ");

      qInfo(gui).noquote().nospace() << QString(hostname + ipaddr + tr("port %3.")).
        arg(hostNameList.join(", ")).arg(hostIpList.join(", ")).arg(serverPort());
    }
    else
    {
      QString hostname = hostNameList.size() > 1 ? tr("Server is listening on hostnames %1 ") :
                         tr("Server is listening on hostname %1 ");

      QString ipaddr = hostIpList.size() > 1 ? tr("(IP addresses %2) ") : tr("(IP address %2) ");

      qInfo(gui).noquote().nospace()
        << QString(hostname + ipaddr + tr("port <span style=\"color: #ff0000; font-weight:bold\">%3</span>.")).
        arg(BLUESPAN + hostNameList.join(ENDSPAN + ", " + BLUESPAN) + ENDSPAN).
        arg(BLUESPAN + hostIpList.join(ENDSPAN + ", " + BLUESPAN) + ENDSPAN).
        arg(serverPort());
    }
  }

  return retval;
}

void NavServer::incomingConnection(qintptr socketDescriptor)
{
  qDebug() << "Incoming connection";

  // Create a worker and set name
  NavServerWorker *worker = new NavServerWorker(socketDescriptor, nullptr, options);
  worker->setObjectName("SocketWorker-" + QString::number(socketDescriptor));

  // Create new thread and move the worker into the thread context
  // This allows to receive signals in the thread context instead the sender's context
  QThread *workerThread = new QThread(this);
  workerThread->setObjectName("SocketWorkerThread-" + QString::number(socketDescriptor));
  worker->moveToThread(workerThread);

  connect(workerThread, &QThread::started, worker, &NavServerWorker::threadStarted);
  connect(workerThread, &QThread::finished, [ = ]() -> void
        {
          threadFinished(worker);
        });

  // Data reader will send simconnect packages through this connection
  connect(dataReader, &atools::fs::sc::DataReaderThread::postSimConnectData, worker,
          &NavServerWorker::postSimConnectData);
  connect(worker, &NavServerWorker::postWeatherRequest,
          dataReader, &atools::fs::sc::DataReaderThread::setWeatherRequest);

  qDebug() << "Thread" << worker->objectName();
  workerThread->start();

  workers.insert(worker);
}

void NavServer::threadFinished(NavServerWorker *worker)
{
  qDebug() << "Thread" << worker->objectName() << "finished";

  // A thread has finished - lock the list so the thread can be removed from the list
  QMutexLocker locker(&threadsMutex);

  disconnect(dataReader, &atools::fs::sc::DataReaderThread::postSimConnectData, worker,
             &NavServerWorker::postSimConnectData);

  // TODO crashes when connected
  // disconnect(worker, &NavServerWorker::postCommand,
  // dataReader, &atools::fs::sc::DataReaderThread::postCommand);

  workers.remove(worker);

  // Delete once the event loop is called the next time
  worker->deleteLater();
  worker->thread()->deleteLater();
}

bool NavServer::hasConnections() const
{
  QMutexLocker locker(&threadsMutex);
  return workers.size() > 0;
}

} // namespace ns
} // namespace fs
} // namespace atools
