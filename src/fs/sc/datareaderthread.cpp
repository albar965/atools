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

#include "fs/sc/datareaderthread.h"

#include "fs/sc/simconnecthandler.h"
#include "settings/settings.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

DataReaderThread::DataReaderThread(QObject *parent, bool verboseLog)
  : QThread(parent), verbose(verboseLog)
{
  qDebug() << "Datareader started";
  setObjectName("DataReaderThread");
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << "Datareader deleted";
}

void DataReaderThread::connectToSimulator(atools::fs::sc::SimConnectHandler *handler)
{
  int counter = 0;

  emit postLogMessage(tr("Not connected to the simulator. Waiting ..."), false);

  while(!terminate)
  {
    if((counter % reconnectRateSec) == 0)
    {
      if(handler->connect())
      {
        connected = true;
        emit connectedToSimulator();
        emit postLogMessage(tr("Connected to simulator."), false);
        break;
      }

      counter = 0;
    }
    counter++;
    QThread::sleep(1);
  }
}

void DataReaderThread::run()
{
  qDebug() << "Datareader run";

  setupReplay();

  atools::fs::sc::SimConnectHandler handler(verbose);

  if(loadReplayFile == nullptr)
    // Connect to the simulator
    connectToSimulator(&handler);

  int i = 0;

  while(!terminate)
  {
    atools::fs::sc::SimConnectData data;

    if(loadReplayFile != nullptr)
    {
      data.read(loadReplayFile);

      if(data.getStatus() == OK)
      {
        if(loadReplayFile->atEnd())
          loadReplayFile->seek(REPLAY_FILE_DATA_START_OFFSET);

        emit postSimConnectData(data);
      }
      else
      {
        emit postLogMessage(tr("Error reading \"%1\": %2.").
                            arg(loadReplayFilepath).arg(data.getStatusText()), true);
        closeReplay();
      }
    }
    else if(handler.fetchData(data, 200))
    {
      data.setPacketId(i);
      data.setPacketTimestamp(QDateTime::currentDateTime().toTime_t());
      emit postSimConnectData(data);

      if(saveReplayFile != nullptr && saveReplayFile->isOpen())
        data.write(saveReplayFile);

      i++;
    }
    else
    {
      if(handler.getState() != atools::fs::sc::STATEOK)
      {
        connected = false;
        emit disconnectedFromSimulator();

        qWarning() << "Error fetching data from simulator.";

        if(!handler.isSimRunning())
          // Try to reconnect if we lost connection to simulator
          connectToSimulator(&handler);
      }
    }

    if(loadReplayFile != nullptr)
      QThread::msleep(static_cast<float>(updateRate) / static_cast<float>(replaySpeed));
    else
      QThread::msleep(updateRate);
  }

  closeReplay();

  terminate = false; // Allow restart
  connected = false;
  emit disconnectedFromSimulator();
  qDebug() << "Datareader exiting run";
}

void DataReaderThread::setReconnectRateSec(int reconnectSec)
{
  reconnectRateSec = reconnectSec;
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
        emit postLogMessage(tr("Cannot open \"%1\".").arg(loadReplayFilepath), true);
        delete loadReplayFile;
        loadReplayFile = nullptr;
      }
      else
      {
        QDataStream in(loadReplayFile);
        in.setVersion(QDataStream::Qt_5_5);

        quint32 magicNumber, version;

        in >> magicNumber >> version >> replayUpdateRateMs;
        if(magicNumber != REPLAY_FILE_MAGIC_NUMBER)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Is not a replay file - wrong magic number.").
                              arg(loadReplayFilepath), true);
          closeReplay();
          return;
        }
        if(version != REPLAY_FILE_VERSION)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Wrong version.").arg(loadReplayFilepath), true);
          closeReplay();
          return;
        }

        updateRate = replayUpdateRateMs;
        emit postLogMessage(tr("Replaying from \"%1\".").arg(loadReplayFilepath), false);
        emit connectedToSimulator();
      }
    }
    else
    {
      emit postLogMessage(tr("Cannot open \"%1\". File is too small.").arg(loadReplayFilepath), true);
      closeReplay();
      return;
    }
  }
  else if(!saveReplayFilepath.isEmpty())
  {
    saveReplayFile = new QFile(saveReplayFilepath);
    if(!saveReplayFile->open(QIODevice::WriteOnly))
    {
      emit postLogMessage(tr("Cannot open \"%1\".").arg(saveReplayFilepath), true);
      delete saveReplayFile;
      saveReplayFile = nullptr;
    }
    else
    {
      emit postLogMessage(tr("Saving replay to \"%1\".").arg(saveReplayFilepath), false);

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

} // namespace sc
} // namespace fs
} // namespace atools
