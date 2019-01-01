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

#ifndef LITTLENAVCONNECT_DATAREADERTHREAD_H
#define LITTLENAVCONNECT_DATAREADERTHREAD_H

#include "fs/sc/simconnectdata.h"
#include "fs/sc/simconnectreply.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

class QFile;

namespace atools {
namespace fs {
namespace sc {

class ConnectHandler;

/* Actively reads flight simulator data using the simconnect interface in background and sends a
 * signal for each data package. */
class DataReaderThread :
  public QThread
{
  Q_OBJECT

public:
  DataReaderThread(QObject *parent, bool verboseLog);
  virtual ~DataReaderThread();

  void setHandler(atools::fs::sc::ConnectHandler *connectHandler);

  /* Terminate, wait for termination and reset flag afterwards */
  void terminateThread();

  /* Thread will terminate after the next iteration. */
  void setTerminate(bool terminateFlag)
  {
    terminate = terminateFlag;
  }

  /* Read data and send a signal every updateRateMs */
  void setUpdateRate(unsigned int updateRateMs)
  {
    updateRate = updateRateMs;
  }

  /* If simulator connection is lost try to reconnect every reconnectSec seconds. */
  void setReconnectRateSec(int reconnectSec);

  bool isConnected() const
  {
    return connected;
  }

  bool isReconnecting() const
  {
    return reconnecting;
  }

  /* If set all data will be saved into that file too */
  void setSaveReplayFilepath(const QString& value)
  {
    saveReplayFilepath = value;
  }

  /* If set all data will be read from that file and all simulator connections will be ignored */
  void setLoadReplayFilepath(const QString& value)
  {
    loadReplayFilepath = value;
  }

  void setReplaySpeed(int value)
  {
    replaySpeed = std::max(1, value);
  }

  void closeReplay();

  bool isSimconnectAvailable();

  /* Sets a one shot request to fetch on next iteration */
  void setWeatherRequest(atools::fs::sc::WeatherRequest request);

  void setSimconnectOptions(atools::fs::sc::Options value);

  /* What type of handler is set now */
  bool isFsxHandler();
  bool isXplaneHandler();

  atools::fs::sc::ConnectHandler *getHandler() const
  {
    return handler;
  }

signals:
  /* Send on each received data package from the simconnect interface */
  void postSimConnectData(atools::fs::sc::SimConnectData dataPacket);

  void postLogMessage(QString messge, bool warning);

  /* Emitted when a connection was established */
  void connectedToSimulator();

  /* Emitted when disconnected manually or due to error */
  void disconnectedFromSimulator();

private:
  void connectToSimulator();
  virtual void run() override;
  void setupReplay();
  bool fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, atools::fs::sc::Options options);

  atools::fs::sc::ConnectHandler *handler = nullptr;

  /* Have to protect options since they will be modified from outside the thread */
  std::atomic<atools::fs::sc::Options> options;

  int numErrors = 0;
  const int MAX_NUMBER_OF_ERRORS = 50;

  const quint32 REPLAY_FILE_MAGIC_NUMBER = 0XCACF4F27;
  const quint32 REPLAY_FILE_VERSION = 1;
  const int REPLAY_FILE_DATA_START_OFFSET = sizeof(REPLAY_FILE_MAGIC_NUMBER) + sizeof(REPLAY_FILE_VERSION) +
                                            sizeof(quint32);

  const int SIMCONNECT_AI_RADIUS_KM = 200;

  QString saveReplayFilepath, loadReplayFilepath;
  int replaySpeed = 1;
  QFile *saveReplayFile = nullptr, *loadReplayFile = nullptr;
  quint32 replayUpdateRateMs = 500;

  bool terminate = false, verbose = false;
  unsigned int updateRate = 500;
  int reconnectRateSec = 10;
  bool connected = false, reconnecting = false;

  /* Source for packet ids */
  int nextPacketId = 1;

  /* Needed to lock for any modifications of the handler's data (weather) */
  mutable QMutex handlerMutex;

  /* Threads waits on this for each iteration - used to wake up early for weather requests */
  mutable QMutex waitMutex;
  mutable QWaitCondition waitCondition;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // LITTLENAVCONNECT_DATAREADERTHREAD_H
