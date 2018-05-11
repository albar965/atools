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

#include "fs/sc/xpconnecthandler.h"

#include "fs/sc/weatherrequest.h"

#include "fs/sc/simconnectdata.h"

#include <QBuffer>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

XpConnectHandler::XpConnectHandler()
{
  qDebug() << Q_FUNC_INFO;
}

XpConnectHandler::~XpConnectHandler()
{
  qDebug() << Q_FUNC_INFO;
  disconnect();
}

bool XpConnectHandler::connect()
{
  if(sharedMemory.isAttached())
  {
    qDebug() << Q_FUNC_INFO << "Already attached";
    state = STATEOK;
    return true;
  }

  sharedMemory.setKey(atools::fs::sc::SHARED_MEMORY_KEY);
  if(!sharedMemory.attach(QSharedMemory::ReadOnly))
  {
    if(sharedMemory.error() != QSharedMemory::NotFound)
      qWarning() << Q_FUNC_INFO << "Cannot attach" << sharedMemory.errorString() << sharedMemory.error();
    state = OPEN_ERROR;
    return false;
  }
  else
  {
    qInfo() << Q_FUNC_INFO << "Attached to" << sharedMemory.key() << "native" << sharedMemory.nativeKey();
    state = STATEOK;
    return true;
  }
}

bool XpConnectHandler::fetchData(fs::sc::SimConnectData& data, int radiusKm, fs::sc::Options options)
{
  Q_UNUSED(radiusKm);
  Q_UNUSED(options);

  if(!sharedMemory.isAttached())
  {
    state = DISCONNECTED;
    return false;
  }

  if(sharedMemory.lock())
  {
    quint32 size;
    quint32 terminate;
    int prefixSize = sizeof(size) + sizeof(terminate);
    QDataStream stream(QByteArray(static_cast<const char *>(sharedMemory.data()), prefixSize));
    stream >> size;

    if(size > static_cast<quint32>(prefixSize))
    {
      stream >> terminate;

      QBuffer buffer;
      buffer.setData(static_cast<const char *>(sharedMemory.data()), static_cast<int>(size));
      buffer.open(QIODevice::ReadOnly);

      buffer.seek(sizeof(size) + sizeof(terminate));
      data.read(&buffer);
      sharedMemory.unlock();

      if(terminate)
      {
        disconnect();
        return false;
      }

      if(data.isUserAircraftValid() && data.getStatus() == OK)
      {
        if(!(options & atools::fs::sc::FETCH_AI_AIRCRAFT))
          // Have to clear this here since the X-Plane plugin has no configuration option
          data.getAiAircraft().clear();

        return true;
      }
    }
    else
      sharedMemory.unlock();
  }
  else
    qInfo() << Q_FUNC_INFO << "Cannot lock" << sharedMemory.key() << "native" << sharedMemory.nativeKey();

  return false;
}

bool XpConnectHandler::fetchWeatherData(fs::sc::SimConnectData& data)
{
  Q_UNUSED(data);
  return false;
}

void XpConnectHandler::addWeatherRequest(const fs::sc::WeatherRequest& request)
{
  Q_UNUSED(request);
}

const atools::fs::sc::WeatherRequest& XpConnectHandler::getWeatherRequest() const
{
  static atools::fs::sc::WeatherRequest dummy;
  return dummy;
}

bool XpConnectHandler::isSimRunning() const
{
  return state == STATEOK;
}

bool XpConnectHandler::isSimPaused() const
{
  return false;
}

atools::fs::sc::State XpConnectHandler::getState() const
{
  return state;
}

QString XpConnectHandler::getName() const
{
  return QLatin1Literal("XpConnect");
}

void XpConnectHandler::disconnect()
{
  bool result = sharedMemory.detach();
  qDebug() << Q_FUNC_INFO << "result" << result;
  state = DISCONNECTED;
}

bool XpConnectHandler::isLoaded() const
{
  return true;
}

} // namespace sc
} // namespace fs
} // namespace atools
