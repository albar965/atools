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

  atools::fs::sc::SimConnectHandler handler(verbose);

  // Connect to the simulator
  connectToSimulator(&handler);

  int i = 0;

  while(!terminate)
  {
    atools::fs::sc::SimConnectData data;

    if(handler.fetchData(data, 200))
    {
      data.setPacketId(i);
      data.setPacketTimestamp(QDateTime::currentDateTime().toTime_t());
      emit postSimConnectData(data);
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
    QThread::msleep(updateRate);
  }
  terminate = false; // Allow restart
  connected = false;
  emit disconnectedFromSimulator();
  qDebug() << "Datareader exiting run";
}

void DataReaderThread::setReconnectRateSec(int reconnectSec)
{
  reconnectRateSec = reconnectSec;
}

} // namespace sc
} // namespace fs
} // namespace atools
