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
#include <time.h>
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
#include <QFile>

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
  qInfo() << Q_FUNC_INFO;
}

std::string fetchTrace(const cpptrace::stacktrace& trace)
{
#ifndef DISABLE_CRASHHANDLER
#ifdef QT_DEBUG
  // Do not use raw offsets for debug builds
  Q_UNUSED(trace)
  std::ostringstream stream;
  cpptrace::generate_trace(0, 500).print(stream, false);
  stream << std::ends;
  return stream.str();
#else
  // Print raw offsets which can be resolved using
  // addr2line -e littlenavmap -f -C -p 0x283c6
  std::ostringstream stream;
  stream << "Timestamp:" << time(nullptr) << "\n";
  for(auto it = trace.begin(); it != trace.end(); ++it)
  {
    cpptrace::stacktrace_frame frame = *it;
    stream << frame.filename << " 0x" << std::hex << frame.object_address << std::endl;
  }
  stream << std::ends;
  return stream.str();
#endif
#else
  Q_UNUSED(trace)
  return std::string();
#endif
}

void printTrace(QDebug out, const char*funcInfo, const char *file, int line, const QString& message)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << funcInfo << " " << file << ":" << line << " " << message << endl;

#ifndef DISABLE_CRASHHANDLER
  std::ostringstream stream;
  cpptrace::stacktrace trace = cpptrace::generate_trace(0, 500);
  for(auto it = trace.begin(); it != trace.end(); ++it)
  {
    cpptrace::stacktrace_frame frame = *it;
    out << QString::fromStdString(frame.filename) << " 0x" << hex << frame.object_address << endl;
  }
  stream << std::ends;
  out << QString::fromStdString(stream.str());
#endif
}

void printTraceDebug(const char*funcInfo, const char *file, int line, const QString& message)
{
  printTrace(qDebug(), funcInfo, file, line, message);
}

void printTraceInfo(const char*funcInfo, const char *file, int line, const QString& message)
{
  printTrace(qInfo(), funcInfo, file, line, message);
}

void printTraceWarning(const char*funcInfo, const char *file, int line, const QString& message)
{
  printTrace(qWarning(), funcInfo, file, line, message);
}

void printTraceCritical(const char*funcInfo, const char*file, int line, const QString& message)
{
  printTrace(qCritical(), funcInfo, file, line, message);
}

void setStackTraceLog(const QString& logFilename)
{
#ifndef DISABLE_CRASHHANDLER
  QString utf8Filename = atools::nativeCleanPath(logFilename);
  qDebug() << Q_FUNC_INFO << utf8Filename;

  // Need to use latin1 since open() treats the filename as is
  filename = utf8Filename.toLocal8Bit();
#endif
}

#ifndef DISABLE_CRASHHANDLER
/* Print message using signal safe methods */
static void printSignalMessage(int fileHandle, const char *message)
{
  write(fileHandle, message, strlen(message));

  if(fileHandle != STDERR_FILENO)
    write(STDERR_FILENO, message, strlen(message));
}

/* Open file using signal safe methods. Returns file handle or STDERR_FILENO. */
static int openSignalOutput()
{
  int fileHandle = STDERR_FILENO;
  if(!filename.isEmpty())
  {
#ifdef Q_OS_LINUX
    int openFlag = O_WRONLY | O_TRUNC | O_CREAT | O_FSYNC;
#endif

#ifdef Q_OS_WIN32
    int openFlag = O_WRONLY | O_TRUNC | O_CREAT;
#endif
    fileHandle = open(filename.constData(), openFlag, 0666);

    if(fileHandle == -1)
    {
      printSignalMessage(STDERR_FILENO, "Error opening file \"");
      printSignalMessage(STDERR_FILENO, filename.data());
      printSignalMessage(STDERR_FILENO, "\". Reason: ");
      printSignalMessage(STDERR_FILENO, strerror(errno));
      printSignalMessage(STDERR_FILENO, "\n");
      fileHandle = STDERR_FILENO;
    }
    else
    {
      printSignalMessage(STDERR_FILENO, "Opened file \"");
      printSignalMessage(STDERR_FILENO, filename.data());
      printSignalMessage(STDERR_FILENO, "\".\n");
    }
  }
  return fileHandle;
}

/* Close file using signal safe methods */
static void closeSignalOutput(int fileHandle)
{
  if(fileHandle != STDERR_FILENO)
    close(fileHandle);
}

// ================================================================================
// Windows exception filter - running in limited context

#if defined(Q_OS_WIN32)
static LONG WINAPI windowsExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
  int filehandle = openSignalOutput();

  printSignalMessage(filehandle, "windowsExceptionFilter\n");

  switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      printSignalMessage(filehandle, "Error: EXCEPTION_ACCESS_VIOLATION\n");
      break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      printSignalMessage(filehandle, "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
      break;

    case EXCEPTION_BREAKPOINT:
      printSignalMessage(filehandle, "Error: EXCEPTION_BREAKPOINT\n");
      break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
      printSignalMessage(filehandle, "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
      break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
      break;

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
      break;

    case EXCEPTION_FLT_INEXACT_RESULT:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
      break;

    case EXCEPTION_FLT_INVALID_OPERATION:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
      break;

    case EXCEPTION_FLT_OVERFLOW:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_OVERFLOW\n");
      break;

    case EXCEPTION_FLT_STACK_CHECK:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_STACK_CHECK\n");
      break;

    case EXCEPTION_FLT_UNDERFLOW:
      printSignalMessage(filehandle, "Error: EXCEPTION_FLT_UNDERFLOW\n");
      break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
      printSignalMessage(filehandle, "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
      break;

    case EXCEPTION_IN_PAGE_ERROR:
      printSignalMessage(filehandle, "Error: EXCEPTION_IN_PAGE_ERROR\n");
      break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      printSignalMessage(filehandle, "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
      break;

    case EXCEPTION_INT_OVERFLOW:
      printSignalMessage(filehandle, "Error: EXCEPTION_INT_OVERFLOW\n");
      break;

    case EXCEPTION_INVALID_DISPOSITION:
      printSignalMessage(filehandle, "Error: EXCEPTION_INVALID_DISPOSITION\n");
      break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      printSignalMessage(filehandle, "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
      break;

    case EXCEPTION_PRIV_INSTRUCTION:
      printSignalMessage(filehandle, "Error: EXCEPTION_PRIV_INSTRUCTION\n");
      break;

    case EXCEPTION_SINGLE_STEP:
      printSignalMessage(filehandle, "Error: EXCEPTION_SINGLE_STEP\n");
      break;

    case EXCEPTION_STACK_OVERFLOW:
      printSignalMessage(filehandle, "Error: EXCEPTION_STACK_OVERFLOW\n");
      break;

    default:
      printSignalMessage(filehandle, "Error: Unrecognized Exception\n");
      break;
  }

  // Have to use signal handler unsafe method since libuwind is not available for all platforms
  printSignalMessage(filehandle, fetchTrace(cpptrace::generate_trace(0, 500)).data());
  printSignalMessage(filehandle, "windowsExceptionFilter exit\n");
  closeSignalOutput(filehandle);

  return EXCEPTION_EXECUTE_HANDLER;
}

#elif defined(Q_OS_LINUX)

// Posix handler - running in limited signal context
static void posixSignalHandler(int sig, siginfo_t *siginfo, void *context)
{
  Q_UNUSED(context)

  int filehandle = openSignalOutput();
  printSignalMessage(filehandle, Q_FUNC_INFO);
  printSignalMessage(filehandle, " entry\n");

  switch(sig)
  {
    case SIGSEGV:
      printSignalMessage(filehandle, "Caught SIGSEGV: Segmentation Fault\n");
      break;

    case SIGINT:
      printSignalMessage(filehandle, "Caught SIGINT: Interactive attention signal, (usually ctrl+c)\n");
      break;

    case SIGFPE:
      switch(siginfo->si_code)
      {
        case FPE_INTDIV:
          printSignalMessage(filehandle, "Caught SIGFPE: (integer divide by zero)\n");
          break;
        case FPE_INTOVF:
          printSignalMessage(filehandle, "Caught SIGFPE: (integer overflow)\n");
          break;
        case FPE_FLTDIV:
          printSignalMessage(filehandle, "Caught SIGFPE: (floating-point divide by zero)\n");
          break;
        case FPE_FLTOVF:
          printSignalMessage(filehandle, "Caught SIGFPE: (floating-point overflow)\n");
          break;
        case FPE_FLTUND:
          printSignalMessage(filehandle, "Caught SIGFPE: (floating-point underflow)\n");
          break;
        case FPE_FLTRES:
          printSignalMessage(filehandle, "Caught SIGFPE: (floating-point inexact result)\n");
          break;
        case FPE_FLTINV:
          printSignalMessage(filehandle, "Caught SIGFPE: (floating-point invalid operation)\n");
          break;
        case FPE_FLTSUB:
          printSignalMessage(filehandle, "Caught SIGFPE: (subscript out of range)\n");
          break;
        default:
          printSignalMessage(filehandle, "Caught SIGFPE: Arithmetic Exception\n");
          break;
      }
      break;

    case SIGILL:
      switch(siginfo->si_code)
      {
        case ILL_ILLOPC:
          printSignalMessage(filehandle, "Caught SIGILL: (illegal opcode)\n");
          break;
        case ILL_ILLOPN:
          printSignalMessage(filehandle, "Caught SIGILL: (illegal operand)\n");
          break;
        case ILL_ILLADR:
          printSignalMessage(filehandle, "Caught SIGILL: (illegal addressing mode)\n");
          break;
        case ILL_ILLTRP:
          printSignalMessage(filehandle, "Caught SIGILL: (illegal trap)\n");
          break;
        case ILL_PRVOPC:
          printSignalMessage(filehandle, "Caught SIGILL: (privileged opcode)\n");
          break;
        case ILL_PRVREG:
          printSignalMessage(filehandle, "Caught SIGILL: (privileged register)\n");
          break;
        case ILL_COPROC:
          printSignalMessage(filehandle, "Caught SIGILL: (coprocessor error)\n");
          break;
        case ILL_BADSTK:
          printSignalMessage(filehandle, "Caught SIGILL: (internal stack error)\n");
          break;
        default:
          printSignalMessage(filehandle, "Caught SIGILL: Illegal Instruction\n");
          break;
      }
      break;

    case SIGTERM:
      printSignalMessage(filehandle, "Caught SIGTERM: a termination request was sent to the program\n");
      break;

    case SIGABRT:
      printSignalMessage(filehandle, "Caught SIGABRT: usually caused by an abort() or assert()\n");
      break;

    default:
      break;
  }

  // Have to use signal handler unsafe method since libuwind is not available for all platforms
  printSignalMessage(filehandle, fetchTrace(cpptrace::generate_trace(0, 500)).data());
  printSignalMessage(filehandle, Q_FUNC_INFO);
  printSignalMessage(filehandle, " exit\n");
  closeSignalOutput(filehandle);

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

void clearStackTrace(const QString& filename)
{
  if(QFile::remove(filename))
    qInfo() << Q_FUNC_INFO << "Success removing stacktrace file" << filename;
  else
    qInfo() << Q_FUNC_INFO << "Stacktrace file not removed" << filename;
}

} // namespace crashhandler
} // namespace util
} // namespace atools
