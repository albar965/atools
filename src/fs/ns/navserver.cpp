/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "util/htmlbuilder.h"

#include <QNetworkInterface>
#include <QHostInfo>

namespace atools {
namespace fs {
namespace ns {

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

  // hostname/ip/v6
  struct Host
  {
    Host(QString nameParam, QString ipParam, bool isIpv6Param)
      : name(nameParam), ip(ipParam), ipv6(isIpv6Param)
    {
    }

    QString name, ip;
    bool ipv6;
  };

  QList<Host> hosts;

  // Listen on all network interfaces =================================
  bool retval = listen(QHostAddress::Any, static_cast<quint16>(port));

  qDebug() << "retval" << retval << "serverAddress" << serverAddress()
           << "localHostName" << QHostInfo::localHostName() << "localDomainName" << QHostInfo::localDomainName();

  if(retval)
  {
    QHostAddress localhostIpv4, localhostIpv6;
    // Collect usable hostnames and IPs from all interfaces =================================
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for(const QHostAddress& hostAddr : addresses)
    {
      if(hostAddr.isNull() || hostAddr.isBroadcast() || hostAddr.isMulticast())
        continue;

      if(hostAddr.isLoopback())
      {
        // Add loopback addresses later if nothing else was found
        if(hostAddr.protocol() == QAbstractSocket::IPv4Protocol)
          localhostIpv4 = hostAddr;
        else if(hostAddr.protocol() == QAbstractSocket::IPv6Protocol)
          localhostIpv6 = hostAddr;
        continue;
      }

      // No loopback and only IPv4 and IPv6
      if(hostAddr.protocol() == QAbstractSocket::IPv4Protocol ||
         (hostAddr.protocol() == QAbstractSocket::IPv6Protocol && !hostAddr.isLinkLocal()))
      {
        QString name = QHostInfo::fromName(hostAddr.toString()).hostName();
        hosts.append(Host(name, hostAddr.toString(), hostAddr.protocol() == QAbstractSocket::IPv6Protocol));

        qDebug() << "Using address" << hostAddr.toString() << "name" << name;
      }
      else
        qDebug() << "Ignoring address" << hostAddr.toString();
    }

    // Ensure IPv4 in front
    std::sort(hosts.begin(), hosts.end(), [](const Host& host1, const Host& host2) {
            return host1.ipv6 < host2.ipv6;
          });

    // Add localhost if nothing was found =================================
    if(hosts.isEmpty())
    {
      if(!localhostIpv4.isNull())
        hosts.append(Host("localhost", localhostIpv4.toString(), false));
      if(!localhostIpv6.isNull())
        hosts.append(Host("localhost", localhostIpv6.toString(), true));
    }

    if(!hosts.isEmpty())
    {
      if(options.testFlag(HIDE_HOST))
        qInfo(gui).noquote().nospace() << tr("Server is listening.");
      else
      {
        // Log addresses to output window =============================
        // Header
        atools::util::HtmlBuilder html;
        if(hosts.size() > 1)
          html.text(tr("Server is listening on hostnames (IP-addresses) on port "));
        else
          html.text(tr("Server is listening on hostname (IP-address) on port "));
        html.text(QString::number(serverPort()), atools::util::html::BOLD, QColor(Qt::red)).text(tr(":"));
        qInfo(gui).noquote().nospace() << html.getHtml();

        // Addresses
        int num = 1;
        for(const Host& host : std::as_const(hosts))
        {
          html.clear();

          if(host.ipv6)
            html.text(tr("%1. IPv6 ").arg(num++));
          else
            html.text(tr("%1. IPv4 ").arg(num++));

          // Name
          html.text(tr("%1 ").arg(host.name), atools::util::html::BOLD, QColor(Qt::blue));

          // Address
          html.text(tr(" (%1)").arg(host.ip), atools::util::html::SMALL, QColor(Qt::blue));
          qInfo(gui).noquote().nospace() << html.getHtml();
        }
        qInfo(gui).noquote().nospace() << tr("Use the mouse to select a hostname or IP-address.");
        qInfo(gui).noquote().nospace() << tr("Then copy the selected text to the clipboard using the context menu.");
      }
    }
    else
      qCritical(gui).noquote().nospace() << tr("Error: No network found.");
  }
  else
    qCritical(gui).noquote().nospace() << tr("Unable to start the server: %1.").arg(errorString());

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
  connect(workerThread, &QThread::finished, this, [ = ]() -> void
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
