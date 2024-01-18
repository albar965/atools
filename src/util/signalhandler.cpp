/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "signalhandler.h"

#ifdef Q_OS_LINUX
#include <csignal>
#include <sys/socket.h>
#include <QSocketNotifier>
#include <QDebug>
#include <unistd.h>
#endif

#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"

namespace atools {
namespace util {

#ifdef Q_OS_LINUX

int SignalHandler::sigHupFd[2];
int SignalHandler::sigTermFd[2];
int SignalHandler::sigIntFd[2];

#endif

SignalHandler *SignalHandler::handlerInstance = nullptr;

const SignalHandler& SignalHandler::instance()
{
  if(handlerInstance == nullptr)
    handlerInstance = new SignalHandler;

  return *handlerInstance;
}

void SignalHandler::deleteInstance()
{
  delete handlerInstance;
  handlerInstance = nullptr;
}

#ifdef Q_OS_LINUX

SignalHandler::SignalHandler()
{
  initSignals();

  // Create  unnamed  pairs  of connected sockets
  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigHupFd))
    qCritical() << Q_FUNC_INFO << "Couldn't create SIGHUP socketpair";

  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigTermFd))
    qCritical() << Q_FUNC_INFO << "Couldn't create SIGTERM socketpair";

  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFd))
    qCritical() << Q_FUNC_INFO << "Couldn't create SIGINT socketpair";

  sigHupNotifier = new QSocketNotifier(sigHupFd[1], QSocketNotifier::Read, this);
  sigTermNotifier = new QSocketNotifier(sigTermFd[1], QSocketNotifier::Read, this);
  siglIntNotifier = new QSocketNotifier(sigIntFd[1], QSocketNotifier::Read, this);

  connect(sigHupNotifier, &QSocketNotifier::activated, this, &SignalHandler::sigHupActivated);
  connect(sigTermNotifier, &QSocketNotifier::activated, this, &SignalHandler::sigTermActivated);
  connect(siglIntNotifier, &QSocketNotifier::activated, this, &SignalHandler::sigIntActivated);
}

SignalHandler::~SignalHandler()
{
  delete sigHupNotifier;
  delete sigTermNotifier;
  delete siglIntNotifier;
}

bool SignalHandler::initSignals()
{
  struct sigaction hupAction, termAction, intAction;

  hupAction.sa_handler = SignalHandler::sigHupHandler;
  sigemptyset(&hupAction.sa_mask);
  hupAction.sa_flags = 0;
  hupAction.sa_flags |= SA_RESTART; // Restart of pending calls is requested

  if(sigaction(SIGHUP, &hupAction, nullptr))
  {
    qCritical() << Q_FUNC_INFO << "Couldn't set signal handler for SIGHUP";
    return false;
  }

  termAction.sa_handler = SignalHandler::sigTermHandler;
  sigemptyset(&termAction.sa_mask);
  termAction.sa_flags = 0;
  termAction.sa_flags |= SA_RESTART; // Restart of pending calls is requested

  if(sigaction(SIGTERM, &termAction, nullptr))
  {
    qCritical() << Q_FUNC_INFO << "Couldn't set signal handler for SIGTERM";
    return false;
  }

  intAction.sa_handler = SignalHandler::sigIntHandler;
  sigemptyset(&intAction.sa_mask);
  intAction.sa_flags = 0;
  intAction.sa_flags |= SA_RESTART; // Restart of pending calls is requested

  if(sigaction(SIGINT, &intAction, nullptr))
  {
    qCritical() << Q_FUNC_INFO << "Couldn't set signal handler for SIGINT";
    return false;
  }

  return true;
}

void SignalHandler::sigHupHandler(int)
{
  char a = 1;
  ::write(sigHupFd[0], &a, sizeof(a));
}

void SignalHandler::sigTermHandler(int)
{
  char a = 1;
  ::write(sigTermFd[0], &a, sizeof(a));
}

void SignalHandler::sigIntHandler(int)
{
  char a = 1;
  ::write(sigIntFd[0], &a, sizeof(a));
}

void SignalHandler::sigTermActivated()
{
  qInfo() << Q_FUNC_INFO;

  sigTermNotifier->setEnabled(false);
  char tmp;
  ::read(sigTermFd[1], &tmp, sizeof(tmp));

  emit sigTermReceived();

  sigTermNotifier->setEnabled(true);
}

void SignalHandler::sigHupActivated()
{
  qInfo() << Q_FUNC_INFO;

  sigHupNotifier->setEnabled(false);
  char tmp;
  ::read(sigHupFd[1], &tmp, sizeof(tmp));

  emit sigHupReceived();

  sigHupNotifier->setEnabled(true);
}

void SignalHandler::sigIntActivated()
{
  qInfo() << Q_FUNC_INFO;

  siglIntNotifier->setEnabled(false);
  char tmp;
  ::read(sigIntFd[1], &tmp, sizeof(tmp));

  emit sigIntReceived();

  siglIntNotifier->setEnabled(true);
}

#endif // Q_OS_LINUX

} // namespace util
} // namespace atools
