/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_LOGGING_LOGGINGUTIL_H
#define ATOOLS_LOGGING_LOGGINGUTIL_H

namespace atools {
namespace logging {

/*
 * Utility to log system information to qInfo.
 */
class LoggingUtil
{
public:
  /*
   * Logs all aspects of QCoreApplication and QSysInfo to the qInfo default
   * channel.
   */
  static void logSystemInformation();

  /*
   * Logs QStandardPaths to the qInfo default channel.
   */
  static void logStandardPaths();

private:
  LoggingUtil()
  {

  }

};

} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGING_LOGGINGUTIL_H
