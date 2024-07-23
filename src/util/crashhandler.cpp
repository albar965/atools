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

#include "util/crashhandler.h"

#ifndef DISABLE_CRASHHANDLER

#include "atools.h"

extern "C"
{
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
}

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

#if defined(Q_OS_LINUX)
extern "C"
{
#include <err.h>
#include <execinfo.h>
}
#endif

#include <cpptrace/cpptrace.hpp>
#include <sstream>
#endif

#include <QDebug>

namespace atools {
namespace util {
namespace crashhandler {

#ifndef DISABLE_CRASHHANDLER
static void setSignalHandler();

static QByteArray filename;
static const int SIGNAL_STACK_SIZE = 1024 * 1024;
#endif

void init()
{
#ifdef DISABLE_CRASHHANDLER
  qInfo() << Q_FUNC_INFO << "Crashhandler disabled";
#else
  qInfo() << Q_FUNC_INFO << "Crashhandler enabled";
  setSignalHandler();
#endif
}

void deInit()
{
}

void printTraceDebug(const char*funcInfo, const char *file, int line, const QString& message)
{
  qDebug().noquote().nospace() << funcInfo << " " << file << ":" << line << " " << message;

#ifndef DISABLE_CRASHHANDLER
  std::ostringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  qDebug().noquote().nospace() << QString::fromStdString(out.str());
#endif
}

void printTraceInfo(const char*funcInfo, const char *file, int line, const QString& message)
{
  qInfo().noquote().nospace() << funcInfo << " " << file << ":" << line << " " << message;

#ifndef DISABLE_CRASHHANDLER
  std::ostringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  qInfo().noquote().nospace() << QString::fromStdString(out.str());
#endif
}

void printTraceWarning(const char*funcInfo, const char *file, int line, const QString& message)
{
  qWarning().noquote().nospace() << funcInfo << " " << file << ":" << line << " " << message;

#ifndef DISABLE_CRASHHANDLER
  std::ostringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  qWarning().noquote().nospace() << QString::fromStdString(out.str());
#endif
}

void printTraceCritical(const char*funcInfo, const char*file, int line, const QString& message)
{
  qCritical().noquote().nospace() << funcInfo << " " << file << ":" << line << " " << message;

#ifndef DISABLE_CRASHHANDLER
  std::ostringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  qCritical().noquote().nospace() << QString::fromStdString(out.str());
#endif
}

void setStackTraceLog(const QString& logFilename)
{
#ifndef DISABLE_CRASHHANDLER
  filename = atools::nativeCleanPath(logFilename).toUtf8();
  qDebug() << Q_FUNC_INFO << filename;
#endif
}

#ifndef DISABLE_CRASHHANDLER
/* Print message using signal safe methods */
static void printSignalMessage(int fh, const char *message)
{
  write(fh, message, strlen(message));

  if(fh != STDERR_FILENO)
    write(STDERR_FILENO, message, strlen(message));
}

/* Open file using signal safe methods. Returns file handle or STDERR_FILENO. */
static int openSignalOutput()
{
  int fh = STDERR_FILENO;
  if(!filename.isEmpty())
  {
#ifdef Q_OS_LINUX
    int openFlag = O_WRONLY | O_TRUNC | O_CREAT | O_FSYNC;
#endif

#ifdef Q_OS_WIN32
    int openFlag = O_WRONLY | O_TRUNC | O_CREAT;
#endif
    fh = open(filename.data(), openFlag, 0666);

    if(fh == -1)
    {
      printSignalMessage(STDERR_FILENO, "Error opening file \"");
      printSignalMessage(STDERR_FILENO, filename.data());
      printSignalMessage(STDERR_FILENO, "\". Reason: ");
      printSignalMessage(STDERR_FILENO, strerror(errno));
      printSignalMessage(STDERR_FILENO, "\n");
      fh = STDERR_FILENO;
    }
    else
    {
      printSignalMessage(STDERR_FILENO, "Opened file \"");
      printSignalMessage(STDERR_FILENO, filename.data());
      printSignalMessage(STDERR_FILENO, "\".\n");
    }
  }
  return fh;
}

/* Close file using signal safe methods */
static void closeSignalOutput(int fh)
{
  if(!filename.isEmpty())
    close(fh);
}

// ================================================================================
// Windows exception filter - running in limited context

#if defined(Q_OS_WIN32)
static LONG WINAPI windowsExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
  int fh = openSignalOutput();

  printSignalMessage(fh, "windowsExceptionFilter\n");

  switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      printSignalMessage(fh, "Error: EXCEPTION_ACCESS_VIOLATION\n");
      break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      printSignalMessage(fh, "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
      break;

    case EXCEPTION_BREAKPOINT:
      printSignalMessage(fh, "Error: EXCEPTION_BREAKPOINT\n");
      break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
      printSignalMessage(fh, "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
      break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
      break;

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
      break;

    case EXCEPTION_FLT_INEXACT_RESULT:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
      break;

    case EXCEPTION_FLT_INVALID_OPERATION:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
      break;

    case EXCEPTION_FLT_OVERFLOW:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_OVERFLOW\n");
      break;

    case EXCEPTION_FLT_STACK_CHECK:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_STACK_CHECK\n");
      break;

    case EXCEPTION_FLT_UNDERFLOW:
      printSignalMessage(fh, "Error: EXCEPTION_FLT_UNDERFLOW\n");
      break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
      printSignalMessage(fh, "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
      break;

    case EXCEPTION_IN_PAGE_ERROR:
      printSignalMessage(fh, "Error: EXCEPTION_IN_PAGE_ERROR\n");
      break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      printSignalMessage(fh, "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
      break;

    case EXCEPTION_INT_OVERFLOW:
      printSignalMessage(fh, "Error: EXCEPTION_INT_OVERFLOW\n");
      break;

    case EXCEPTION_INVALID_DISPOSITION:
      printSignalMessage(fh, "Error: EXCEPTION_INVALID_DISPOSITION\n");
      break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      printSignalMessage(fh, "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
      break;

    case EXCEPTION_PRIV_INSTRUCTION:
      printSignalMessage(fh, "Error: EXCEPTION_PRIV_INSTRUCTION\n");
      break;

    case EXCEPTION_SINGLE_STEP:
      printSignalMessage(fh, "Error: EXCEPTION_SINGLE_STEP\n");
      break;

    case EXCEPTION_STACK_OVERFLOW:
      printSignalMessage(fh, "Error: EXCEPTION_STACK_OVERFLOW\n");
      break;

    default:
      printSignalMessage(fh, "Error: Unrecognized Exception\n");
      break;
  }

  // Have to use signal handler unsafe method since libuwind is not available for all platforms
  std::stringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  printSignalMessage(fh, out.str().data());
  printSignalMessage(fh, "posixSignalHandler exit\n");
  closeSignalOutput(fh);

  return EXCEPTION_EXECUTE_HANDLER;
}

#elif defined(Q_OS_LINUX)

// Posix handler - running in limited signal context
static void posixSignalHandler(int sig, siginfo_t *siginfo, void *context)
{
  Q_UNUSED(context)

  int fh = openSignalOutput();
  printSignalMessage(fh, Q_FUNC_INFO);
  printSignalMessage(fh, " entry\n");

  switch(sig)
  {
    case SIGSEGV:
      printSignalMessage(fh, "Caught SIGSEGV: Segmentation Fault\n");
      break;

    case SIGINT:
      printSignalMessage(fh, "Caught SIGINT: Interactive attention signal, (usually ctrl+c)\n");
      break;

    case SIGFPE:
      switch(siginfo->si_code)
      {
        case FPE_INTDIV:
          printSignalMessage(fh, "Caught SIGFPE: (integer divide by zero)\n");
          break;
        case FPE_INTOVF:
          printSignalMessage(fh, "Caught SIGFPE: (integer overflow)\n");
          break;
        case FPE_FLTDIV:
          printSignalMessage(fh, "Caught SIGFPE: (floating-point divide by zero)\n");
          break;
        case FPE_FLTOVF:
          printSignalMessage(fh, "Caught SIGFPE: (floating-point overflow)\n");
          break;
        case FPE_FLTUND:
          printSignalMessage(fh, "Caught SIGFPE: (floating-point underflow)\n");
          break;
        case FPE_FLTRES:
          printSignalMessage(fh, "Caught SIGFPE: (floating-point inexact result)\n");
          break;
        case FPE_FLTINV:
          printSignalMessage(fh, "Caught SIGFPE: (floating-point invalid operation)\n");
          break;
        case FPE_FLTSUB:
          printSignalMessage(fh, "Caught SIGFPE: (subscript out of range)\n");
          break;
        default:
          printSignalMessage(fh, "Caught SIGFPE: Arithmetic Exception\n");
          break;
      }
      break;

    case SIGILL:
      switch(siginfo->si_code)
      {
        case ILL_ILLOPC:
          printSignalMessage(fh, "Caught SIGILL: (illegal opcode)\n");
          break;
        case ILL_ILLOPN:
          printSignalMessage(fh, "Caught SIGILL: (illegal operand)\n");
          break;
        case ILL_ILLADR:
          printSignalMessage(fh, "Caught SIGILL: (illegal addressing mode)\n");
          break;
        case ILL_ILLTRP:
          printSignalMessage(fh, "Caught SIGILL: (illegal trap)\n");
          break;
        case ILL_PRVOPC:
          printSignalMessage(fh, "Caught SIGILL: (privileged opcode)\n");
          break;
        case ILL_PRVREG:
          printSignalMessage(fh, "Caught SIGILL: (privileged register)\n");
          break;
        case ILL_COPROC:
          printSignalMessage(fh, "Caught SIGILL: (coprocessor error)\n");
          break;
        case ILL_BADSTK:
          printSignalMessage(fh, "Caught SIGILL: (internal stack error)\n");
          break;
        default:
          printSignalMessage(fh, "Caught SIGILL: Illegal Instruction\n");
          break;
      }
      break;

    case SIGTERM:
      printSignalMessage(fh, "Caught SIGTERM: a termination request was sent to the program\n");
      break;

    case SIGABRT:
      printSignalMessage(fh, "Caught SIGABRT: usually caused by an abort() or assert()\n");
      break;

    default:
      break;
  }

  // Have to use signal handler unsafe method since libuwind is not available for all platforms
  std::stringstream out;
  cpptrace::generate_trace(0, 500).print(out, false);
  out << std::ends;
  printSignalMessage(fh, out.str().data());
  printSignalMessage(fh, Q_FUNC_INFO);
  printSignalMessage(fh, " exit\n");
  closeSignalOutput(fh);

  exit(EXIT_FAILURE);
}

#endif // #elif defined(Q_OS_LINUX)

// Set handler / filter - this runs in program context
void setSignalHandler()
{
  // Set Windows exception filter ==============================
#if defined(Q_OS_WIN32)
  SetUnhandledExceptionFilter(windowsExceptionFilter);
#elif defined(Q_OS_LINUX)

  // Set Posix signal handler ==============================

  // setup alternate stack =============================================
  stack_t ss = {};
  ss.ss_size = SIGNAL_STACK_SIZE;
  ss.ss_sp = malloc(ss.ss_size); // Never freed
  ss.ss_flags = 0;

  if(sigaltstack(&ss, nullptr) != 0)
  {
    qWarning() << Q_FUNC_INFO << "sigaltstack" << strerror(errno);
    return;
  }

  // register signal handlers =============================================
  // First empty sigaction
  struct sigaction sig_action = {};
  sig_action.sa_sigaction = posixSignalHandler;
  sigemptyset(&sig_action.sa_mask);

  sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;

  // Available signals
  // SIGINT    2 /* Interactive attention signal.  */
  // SIGILL    4 /* Illegal instruction.  */
  // SIGABRT   6 /* Abnormal termination.  */
  // SIGFPE    8 /* Erroneous arithmetic operation.  */
  // SIGSEGV   11  /* Invalid access to storage.  */
  // SIGTERM   15  /* Termination request.  */
  // SIGHUP    1 /* Hangup.  */
  // SIGQUIT   3 /* Quit.  */
  // SIGTRAP   5 /* Trace/breakpoint trap.  */
  // SIGKILL   9 /* Killed.  */
  // SIGPIPE   13  /* Broken pipe.  */
  // SIGALRM   14  /* Alarm clock.  */

  if(sigaction(SIGSEGV, &sig_action, nullptr) != 0)
  {
    qWarning() << Q_FUNC_INFO << "sigaction SIGSEGV" << strerror(errno);
    return;
  }

  if(sigaction(SIGFPE, &sig_action, nullptr) != 0)
  {
    qWarning() << Q_FUNC_INFO << "sigaction SIGFPE" << strerror(errno);
    return;
  }

  // Handled by SignalHandler
  // if(sigaction(SIGINT, &sig_action, nullptr) != 0)
  // {
  // qWarning() << Q_FUNC_INFO << "sigaction SIGINT" << strerror(errno);
  // return;
  // }

  if(sigaction(SIGILL, &sig_action, nullptr) != 0)
  {
    qWarning() << Q_FUNC_INFO << "sigaction SIGILL" << strerror(errno);
    return;
  }

  // Handled by SignalHandler
  // if(sigaction(SIGTERM, &sig_action, nullptr) != 0)
  // {
  // qWarning() << Q_FUNC_INFO << "sigaction SIGTERM" << strerror(errno);
  // return;
  // }

  if(sigaction(SIGABRT, &sig_action, nullptr) != 0)
  {
    qWarning() << Q_FUNC_INFO << "sigaction SIGABRT" << strerror(errno);
    return;
  }

#endif // elif defined(Q_OS_LINUX)
}

#endif // #ifndef DISABLE_CRASHHANDLER

} // namespace crashhandler
} // namespace util
} // namespace atools
