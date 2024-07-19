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

#ifndef ATOOLS_CRASHHANDLER_H
#define ATOOLS_CRASHHANDLER_H

#include <QString>

#define ATOOLS_PRINT_STACK_DEBUG(message) (atools::util::crashhandler::printTraceDebug(Q_FUNC_INFO, __FILE__, __LINE__, message))
#define ATOOLS_PRINT_STACK_INFO(message) (atools::util::crashhandler::printTraceInfo(Q_FUNC_INFO, __FILE__, __LINE__, message))
#define ATOOLS_PRINT_STACK_WARNING(message) (atools::util::crashhandler::printTraceWarning(Q_FUNC_INFO, __FILE__, __LINE__, message))

/* Functions for catching signals/exceptions on Linux and Windows.
 * Signal handler print a stack trace to the given file and stderr and exits program with EXIT_FAILURE.
 * Disabled on macOS.
 *
 * This requires https://github.com/jeremy-rifkin/cpptrace .
 */
namespace atools {
namespace util {
namespace crashhandler {

/* Initialize and set signal/exception handler. Errors when setting handler are logged and otherwise ignored. */
void init();
void deInit();

/* Print a stack trace for the current process instruction pointer */
void printTraceDebug(const char *funcInfo, const char *file, int line, const QString& message);
void printTraceInfo(const char *funcInfo, const char *file, int line, const QString& message);
void printTraceWarning(const char *funcInfo, const char *file, int line, const QString& message);

/* Set a log file to write the stack traces in case of signal/exception. stderr is used as default if not set.
 * Can be called after init() */
void setStackTraceLog(const QString& filename);

} // namespace crashhandler
} // namespace util
} // namespace atools

#endif // ATOOLS_CRASHHANDLER_H
