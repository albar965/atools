/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_SIGNALHANDLER_H
#define ATOOLS_SIGNALHANDLER_H

#include <QObject>

class QSocketNotifier;

namespace atools {
namespace util {

/*
 * Safe singleton Unix signal handler which will send Qt signals when receiving Unix signals SIGHUP, SIGTERM or SIGINT.
 * Only active on Linux/Unix.
 */
class SignalHandler :
  public QObject
{
  Q_OBJECT

public:
  /* Creates instance on demand */
  const static SignalHandler& instance();

  /* Deletes instance */
  static void deleteInstance();

signals:
  /* SIGHUP Hangup detected on controlling terminal or death of controlling process.
   * Inactive on non Unix/Linux systems */
  void sigHupReceived();

  /* SIGTERM Termination signal */
  void sigTermReceived();

  /* SIGINT Interrupt from keyboard o Ctrl+C */
  void sigIntReceived();

private:
#ifdef Q_OS_LINUX
  explicit SignalHandler();
  virtual ~SignalHandler() override;

  /* Unix signal handlers. Write one byte to the first of the connected socket pairs. */
  static void sigHupHandler(int unused);
  static void sigTermHandler(int unused);
  static void sigIntHandler(int);

  /* Initialize Unix signal handlers */
  static bool initSignals();

  /* Qt signal handlers. Called by QSocketNotifier and read one byte to the first of the connected socket pairs*/
  void sigHupActivated();
  void sigTermActivated();
  void sigIntActivated();

  // Socket pair file descriptors. 0 is written by signal handler and 1 is read by Qt code
  static int sigHupFd[2], sigTermFd[2], sigIntFd[2];

  // Notifiers activated on data sent through sockets
  QSocketNotifier *sigHupNotifier = nullptr, *sigTermNotifier = nullptr, *siglIntNotifier = nullptr;

#endif

  static SignalHandler *handlerInstance;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_SIGNALHANDLER_H
