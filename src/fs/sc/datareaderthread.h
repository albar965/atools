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

#ifndef LITTLENAVCONNECT_DATAREADERTHREAD_H
#define LITTLENAVCONNECT_DATAREADERTHREAD_H

#include "fs/sc/simconnectdata.h"

#include <QThread>

class QFile;

namespace atools {
namespace fs {
namespace sc {

class SimConnectHandler;

/* Actively reads flight simulator data using the simconnect interface in background and sends a
 * signal for each data package. */
class DataReaderThread :
  public QThread
{
  Q_OBJECT

public:
  DataReaderThread(QObject *parent, bool verboseLog);
  virtual ~DataReaderThread();

  /* Thread will terminate after the next iteration. */
  void setTerminate(bool terminateFlag = true)
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

  static bool isSimconnectAvailable()
  {
#ifdef SIMCONNECT_DUMMY
    return false;

#else
    return true;

#endif
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
  void connectToSimulator(atools::fs::sc::SimConnectHandler *handler);
  virtual void run() override;
  void setupReplay();

  const quint32 REPLAY_FILE_MAGIC_NUMBER = 0XCACF4F27;
  const quint32 REPLAY_FILE_VERSION = 1;
  const int REPLAY_FILE_DATA_START_OFFSET = sizeof(REPLAY_FILE_MAGIC_NUMBER) + sizeof(REPLAY_FILE_VERSION) +
                                            sizeof(quint32);

  QString saveReplayFilepath, loadReplayFilepath;
  int replaySpeed = 1;
  QFile *saveReplayFile = nullptr, *loadReplayFile = nullptr;
  quint32 replayUpdateRateMs = 500;

  bool terminate = false, verbose = false;
  unsigned int updateRate = 500;
  int reconnectRateSec = 10;
  bool connected = false, reconnecting = false;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // LITTLENAVCONNECT_DATAREADERTHREAD_H
