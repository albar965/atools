/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/ns/navserverworker.h"
#include "fs/ns/navservercommon.h"
#include "fs/sc/simconnectreply.h"

#include <QThread>
#include <QTcpSocket>

namespace atools {
namespace fs {
namespace ns {

NavServerWorker::NavServerWorker(qintptr socketDescriptor, NavServer *parent,
                                 atools::fs::ns::NavServerOptions optionFlags)
  : QObject(parent), socketDescr(socketDescriptor), options(optionFlags)
{
  qDebug() << "NavServerWorker created" << QThread::currentThread()->objectName();
}

NavServerWorker::~NavServerWorker()
{
  qDebug() << "NavServerWorker destructor" << QThread::currentThread()->objectName();
}

void NavServerWorker::threadStarted()
{
  qDebug() << "NavServerWorker threadStarted" << QThread::currentThread()->objectName();

  if(socket == nullptr)
  {
    socket = new QTcpSocket();
    connect(socket, &QTcpSocket::disconnected, this, &NavServerWorker::socketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NavServerWorker::readyReadReplyFromSocket);
  }

  if(!socket->setSocketDescriptor(socketDescr, QAbstractSocket::ConnectedState, QIODevice::ReadWrite))
  {
    qCritical(gui).noquote().nospace() << tr("Error creating network socket: %1.").arg(socket->errorString());
    return;
  }

  peerAddr = socket->peerAddress().toString();
  hostInfo = QHostInfo::fromName(peerAddr);

  qInfo(gui).noquote().nospace() << tr("Connection from %1 (%2).").arg(hostInfo.hostName()).arg(peerAddr);

  qDebug() << "NavServerWorker Connection from " << hostInfo.hostName() << " (" << peerAddr << ") "
           << "port " << socket->peerPort();
}

void NavServerWorker::socketDisconnected()
{
  qInfo(gui).noquote().nospace() << tr("Connection from %1 (%2) closed.").
    arg(hostInfo.hostName()).arg(peerAddr);

  socket->deleteLater();
  socket = nullptr;
  thread()->exit();
}

void NavServerWorker::readyReadReplyFromSocket()
{
  if(options & VERBOSE)
    qDebug() << "NavServerWorker::readyReadReply enter";

  // Read while data is available
  while(socket->bytesAvailable())
  {
    if(options & VERBOSE)
      qDebug() << "NavServerWorker Ready read" << QThread::currentThread()->objectName();

    // Read the reply from client
    atools::fs::sc::SimConnectReply reply;
    if(!reply.read(socket))
      // Reply not fully read
      handleDroppedPackages(tr("Incomplete reply"));

    if(reply.getStatus() != atools::fs::sc::OK)
    {
      // Not fully read or malformed  content
      qWarning(gui).noquote().nospace() << tr("Error reading reply: %1. Closing connection.").
        arg(reply.getStatusText());
      socket->abort();
    }

    if(options & VERBOSE)
      qDebug() << "NavServerWorker readyReadReply packet id" << reply.getPacketId();

    if(reply.getCommand() == atools::fs::sc::CMD_WEATHER_REQUEST)
    {
      if(options & VERBOSE)
        qDebug() << "NavServerWorker::readyReadReply got weather request";

      // Pass weather request from client to data reader
      emit postWeatherRequest(reply.getWeatherRequest());
    }
    else
    {
      if(options & VERBOSE)
        qDebug() << "NavServerWorker readyReadReply" << QThread::currentThread()->objectName()
                 << "last ids" << lastPacketIds;

      // Normal reply - remove id from sent list
      lastPacketIds.remove(reply.getPacketId());
    }
  }
  if(options & VERBOSE)
    qDebug() << "NavServerWorker::readyReadReply leave";
}

void NavServerWorker::postSimConnectData(atools::fs::sc::SimConnectData dataPacket)
{
  if(options & VERBOSE)
    qDebug() << "NavServerWorker postSimConnectData" << QThread::currentThread()->objectName()
             << "last ids" << lastPacketIds;

  if(!dataPacket.getMetars().isEmpty())
  {
    if(options & VERBOSE)
      qDebug() << "NavServerWorker::postSimConnectData metars num " << dataPacket.getMetars().size();

    if(dataPacket.getUserAircraftConst().getPosition().isValid())
      qWarning() << "Aircraft and metar mixed";
  }

  if(lastPacketIds.size() > 1 && dataPacket.getPacketId() > 0)
  {
    // No reply received in the meantime - count it as dropped package and do not send a new package
    handleDroppedPackages(tr("Missing reply"));
    return;
  }

  if(inPost)
    // We're already posting
    qCritical() << "Nested post";

  if(dataPacket.getPacketId() > 0)
    // Insert packet id in sent list if this is not a weather request
    lastPacketIds.insert(dataPacket.getPacketId());

  inPost = true;

  int written;
  written = dataPacket.write(socket);
  if(dataPacket.getStatus() != atools::fs::sc::OK)
    qWarning(gui).noquote().nospace() << tr("Error writing data: %1.").arg(dataPacket.getStatusText());

  if(!socket->flush())
    qWarning() << "NavServerWorker Reply to client not flushed";

  if(options & VERBOSE)
    qDebug() << "NavServerWorker written" << written << "flush" << flush << "id" << dataPacket.getPacketId();

  inPost = false;
}

void NavServerWorker::handleDroppedPackages(const QString& reason)
{
  droppedPackages++;
  if(droppedPackages > MAX_DROPPED_PACKAGES)
  {
    qWarning(gui).noquote().nospace() << tr("Dropped more than %1 packages. Reason: %2. "
                                            "Increase update time interval.").
      arg(MAX_DROPPED_PACKAGES).arg(reason);

    droppedPackages = 0;

    if(lastPacketIds.size() > 5000)
      lastPacketIds.clear();
  }
  qWarning() << "No reply - ignoring package. Currently dropped" << droppedPackages << "Reason:" << reason;
}

} // namespace ns
} // namespace fs
} // namespace atools
