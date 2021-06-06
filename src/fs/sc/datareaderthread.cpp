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

#include "fs/sc/datareaderthread.h"

#include "fs/sc/simconnecthandler.h"
#include "fs/sc/xpconnecthandler.h"
#include "settings/settings.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDataStream>
#include <QApplication>

namespace atools {
namespace fs {
namespace sc {

DataReaderThread::DataReaderThread(QObject *parent, bool verboseLog)
  : QThread(parent), verbose(verboseLog)
{
  qDebug() << Q_FUNC_INFO;
  setObjectName("DataReaderThread");

  options = atools::fs::sc::FETCH_AI_AIRCRAFT | atools::fs::sc::FETCH_AI_BOAT;
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << Q_FUNC_INFO;
}

void DataReaderThread::setHandler(ConnectHandler *connectHandler)
{
  qDebug() << Q_FUNC_INFO << connectHandler->getName();
  qDebug() << "SimConnect available:" << (handler != nullptr ? handler->isLoaded() : false);

  handler = connectHandler;

}

void DataReaderThread::connectToSimulator()
{
  int counter = 0;

  if(!handler->isLoaded())
  {
    QString msg = tr("No flight simulator installation found. SimConnect not loaded.");
    emit postStatus(atools::fs::sc::OK, msg);
    emit postLogMessage(msg, false, false);
  }
  else
  {
    QString msg = tr("Not connected to the simulator. Waiting ...");
    emit postStatus(atools::fs::sc::OK, msg);
    emit postLogMessage(msg, false, false);

    reconnecting = true;
    while(!terminate)
    {
      if((counter % reconnectRateSec) == 0)
      {
        if(handler->connect())
        {
          connected = true;
          emit connectedToSimulator();
          QString msg = tr("Connected to simulator.");
          emit postStatus(atools::fs::sc::OK, msg);
          emit postLogMessage(msg, false, false);
          break;
        }

        counter = 0;
      }
      counter++;
      QThread::sleep(1);
    }
  }
  reconnecting = false;
}

void DataReaderThread::run()
{
  qDebug() << Q_FUNC_INFO << "update rate" << updateRate;

  setupReplay();

  // Try to connect first ============================================

  if(loadReplayFile == nullptr)
    // Connect to the simulator
    connectToSimulator();
  else
    // Using replay is always connected
    connected = true;

  qDebug() << "Datareader connected";

  waitMutex.lock();

  // Main loop  ============================================
  while(!terminate)
  {
    atools::fs::sc::SimConnectData data;
    atools::fs::sc::Options opts = options;

    if(loadReplayFile != nullptr)
    {
      // Do replay ============================================
      data.read(loadReplayFile);

      if(data.getStatus() == OK)
      {
        if(loadReplayFile->atEnd())
          loadReplayFile->seek(REPLAY_FILE_DATA_START_OFFSET);

        // Remove boat and ship traffic depending on settings for testing purposes
        QVector<SimConnectAircraft>& aiAircraft = data.getAiAircraft();
        if(!(opts & atools::fs::sc::FETCH_AI_AIRCRAFT))
        {
          QVector<SimConnectAircraft>::iterator it =
            std::remove_if(aiAircraft.begin(), aiAircraft.end(), [](const SimConnectAircraft& aircraft) -> bool
                {
                  return !aircraft.isUser() && !aircraft.isAnyBoat();
                });
          if(it != aiAircraft.end())
            aiAircraft.erase(it, aiAircraft.end());
        }

        if(!(opts & atools::fs::sc::FETCH_AI_BOAT))
        {
          QVector<SimConnectAircraft>::iterator it =
            std::remove_if(aiAircraft.begin(), aiAircraft.end(), [](const SimConnectAircraft& aircraft) -> bool
                {
                  return !aircraft.isUser() && aircraft.isAnyBoat();
                });
          if(it != aiAircraft.end())
            aiAircraft.erase(it, aiAircraft.end());
        }

        emit postSimConnectData(data);
      }
      else
      {
        emit postStatus(data.getStatus(), data.getStatusText());
        emit postLogMessage(tr("Error reading \"%1\": %2.").
                            arg(loadReplayFilepath).arg(data.getStatusText()), false, true);
        closeReplay();
      }
    } // if(loadReplayFile != nullptr)
    else if(fetchData(data, aiFetchRadiusKm, opts))
    {
      // Data fetched from simconnect - send to client ============================================
      if(verbose && !data.getMetars().isEmpty())
        qDebug() << "DataReaderThread::run() num metars" << data.getMetars().size();

      emit postSimConnectData(data);

      if(saveReplayFile != nullptr && saveReplayFile->isOpen() && data.getPacketId() > 0)
        // Save only simulator packets, not weather replays
        data.write(saveReplayFile);
    }
    else
    {
      if(handler->getState() != atools::fs::sc::STATEOK)
      {
        // Error fetching data from simconnect ============================================
        connected = false;
        emit disconnectedFromSimulator();

        emit postStatus(data.getStatus(), data.getStatusText());

        qWarning() << "Error fetching data from simulator." << data.getStatusText();

        if(numErrors++ > MAX_NUMBER_OF_ERRORS)
        {
          numErrors = 0;
          emit postLogMessage(tr("Too many errors reading from simulator. Disconnected. "
                                 "Restart <i>%1</i> to try again.").
                              arg(QApplication::applicationName()), false, true);
          break;
        }

        if(!handler->isSimRunning())
          // Try to reconnect if we lost connection to simulator
          connectToSimulator();
      }
      else if(data.getStatus() != OK)
      {
        connected = false;
        emit disconnectedFromSimulator();

        emit postStatus(data.getStatus(), data.getStatusText());

        qWarning() << "Error fetching data from simulator." << data.getStatusText();

        emit postLogMessage(tr("Error reading from simulator: %1. Disconnected. "
                               "Restart <i>%2</i> to try again.").
                            arg(data.getStatusText()).
                            arg(QApplication::applicationName()), false, true);

        if(data.getStatus() == INVALID_MAGIC_NUMBER || data.getStatus() == VERSION_MISMATCH)
        {
          emit postLogMessage(tr("Your installed version of <i>Little Xpconnect</i> "
                                 "is not compatible with this version of <i>%2</i>.").
                              arg(QApplication::applicationName()), false, true);
          emit postLogMessage(tr("Install the latest version of <i>Little Xpconnect</i>."), false, true);
        }

        break;
      }
      // else
      // qWarning() << "No data fetched";
    }

    unsigned long sleepMs = 500;
    if(loadReplayFile != nullptr)
      sleepMs = static_cast<unsigned long>(static_cast<float>(replayUpdateRateMs) /
                                           static_cast<float>(replaySpeed));
    else
      sleepMs = updateRate;

    bool wakeUpSignalled = waitCondition.wait(&waitMutex, sleepMs);
    if(wakeUpSignalled && verbose)
      qDebug() << "DataReaderThread::run wakeUpSignalled";
  }

  closeReplay();

  terminate = false; // Allow restart
  connected = false;
  reconnecting = false;

  qDebug() << "Unlocking wait";
  waitMutex.unlock();

  emit disconnectedFromSimulator();
  qDebug() << Q_FUNC_INFO << "leave";
}

bool DataReaderThread::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, Options options)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "enter";

  if(!handler->isLoaded())
    return true;

  QMutexLocker locker(&handlerMutex);

  bool weatherRequested = handler->getWeatherRequest().isValid();

  bool retval = false;

  if(weatherRequested)
  {
    if(verbose)
      qDebug() << "DataReaderThread::fetchData weather";

    handler->fetchWeatherData(data);

    // Weather requests and reply always have packet id 0
    data.setPacketId(0);

    // Force an empty reply to the client - even if no weather was fetched
    retval = true;
  }
  else
  {
    if(verbose)
      qDebug() << "DataReaderThread::fetchData nextPacketId" << nextPacketId;

    retval = handler->fetchData(data, radiusKm, options);
    data.setPacketId(nextPacketId++);
  }

  data.setPacketTimestamp(QDateTime::currentDateTime().toSecsSinceEpoch());

  if(verbose)
    if(weatherRequested && !data.getMetars().isEmpty())
      qDebug() << "Weather requested and found";

  if(weatherRequested && data.getMetars().isEmpty())
    qWarning() << "Weather requested but noting found";

  handler->addWeatherRequest(WeatherRequest());

  if(verbose)
    qDebug() << Q_FUNC_INFO << "leave";

  return retval;
}

void DataReaderThread::setupReplay()
{
  if(!loadReplayFilepath.isEmpty())
  {
    loadReplayFile = new QFile(loadReplayFilepath);

    if(loadReplayFile->size() > static_cast<qint64>(REPLAY_FILE_DATA_START_OFFSET))
    {
      if(!loadReplayFile->open(QIODevice::ReadOnly))
      {
        emit postLogMessage(tr("Cannot open \"%1\".").arg(loadReplayFilepath), false, true);
        delete loadReplayFile;
        loadReplayFile = nullptr;
      }
      else
      {
        QDataStream in(loadReplayFile);
        in.setVersion(QDataStream::Qt_5_5);
        in.setFloatingPointPrecision(QDataStream::SinglePrecision);

        quint32 magicNumber, version;

        in >> magicNumber >> version >> replayUpdateRateMs;
        if(magicNumber != REPLAY_FILE_MAGIC_NUMBER)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Is not a replay file - wrong magic number.").
                              arg(loadReplayFilepath), false, true);
          closeReplay();
          return;
        }
        if(version != REPLAY_FILE_VERSION)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Wrong version.").arg(loadReplayFilepath), false, true);
          closeReplay();
          return;
        }

        emit postLogMessage(tr("Replaying from \"%1\".").arg(loadReplayFilepath), false, false);
        emit connectedToSimulator();
      }
    }
    else
    {
      emit postLogMessage(tr("Cannot open \"%1\". File is too small.").arg(loadReplayFilepath), false, true);
      closeReplay();
      return;
    }
  }
  else if(!saveReplayFilepath.isEmpty())
  {
    saveReplayFile = new QFile(saveReplayFilepath);
    if(!saveReplayFile->open(QIODevice::WriteOnly))
    {
      emit postLogMessage(tr("Cannot open \"%1\".").arg(saveReplayFilepath), false, true);
      delete saveReplayFile;
      saveReplayFile = nullptr;
    }
    else
    {
      emit postLogMessage(tr("Saving replay to \"%1\".").arg(saveReplayFilepath), false, false);

      // Save file header
      QDataStream out(saveReplayFile);
      out << REPLAY_FILE_MAGIC_NUMBER << REPLAY_FILE_VERSION << static_cast<quint32>(updateRate);
    }
  }
}

void DataReaderThread::closeReplay()
{
  if(saveReplayFile != nullptr)
  {
    saveReplayFile->close();
    delete saveReplayFile;
    saveReplayFile = nullptr;
  }

  if(loadReplayFile != nullptr)
  {
    loadReplayFile->close();
    delete loadReplayFile;
    loadReplayFile = nullptr;
  }
}

bool DataReaderThread::isSimconnectAvailable() const
{
  return handler->isLoaded();
}

bool DataReaderThread::canFetchWeather() const
{
  return handler->canFetchWeather();
}

bool DataReaderThread::isFsxHandler()
{
  return dynamic_cast<atools::fs::sc::SimConnectHandler *>(handler) != nullptr;
}

bool DataReaderThread::isXplaneHandler()
{
  return dynamic_cast<atools::fs::sc::XpConnectHandler *>(handler) != nullptr;
}

void DataReaderThread::setWeatherRequest(atools::fs::sc::WeatherRequest request)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(!canFetchWeather())
  {
    emit postSimConnectData(atools::fs::sc::SimConnectData());
    return;
  }

  if(saveReplayFile != nullptr)
  {
    // Post a dummy weather reply if replaying, do not pass to handler
    emit postSimConnectData(atools::fs::sc::SimConnectData());
    return;
  }

  {
    QMutexLocker locker(&handlerMutex);
    handler->addWeatherRequest(request);
  }

  waitCondition.wakeAll();
}

void DataReaderThread::terminateThread()
{
  setTerminate(true);
  wait();
  setTerminate(false);
}

} // namespace sc
} // namespace fs
} // namespace atools
