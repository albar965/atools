/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_UTIL_FILESYSTEMWATCHER_H
#define ATOOLS_UTIL_FILESYSTEMWATCHER_H

#include "fs/weather/weathertypes.h"

#include <QTimer>

class QFileSystemWatcher;

namespace atools {
namespace util {

/*
 * A better file system watch class which works around for files which are removed, deleted and renamed in
 * the process by checking size and timestamp.
 *
 * Notifications are sent with a delay to catch intermediate changes.
 */
class FileSystemWatcher
  : public QObject
{
  Q_OBJECT

public:
  explicit FileSystemWatcher(QObject *parent, bool verboseLogging);
  virtual ~FileSystemWatcher();

  QString getFileName() const
  {
    return filename;
  }

  /* Set file and start watching */
  void setFilenameAndStart(const QString& value);

  void clear();

  int getMinFileSize() const
  {
    return minFileSize;
  }

  /* Minimum file size needed to generate a signal. Use to avoid half filled files */
  void setMinFileSize(int value)
  {
    minFileSize = value;
  }

  int getDelayMs() const
  {
    return delayMs;
  }

  /* Delay signal */
  void setDelayMs(int value)
  {
    delayMs = value;
  }

signals:
  void fileUpdated(const QString& filename);

private:
  void deleteFsWatcher();
  void createFsWatcher();
  void pathOrFileChanged();
  void fileUpdatedDelayed();
  void setPaths(bool update);

  QString filename;
  QDateTime fileTimestampLastRead;
  qint64 lastFileSizeRead = 0;
  QFileSystemWatcher *fsWatcher = nullptr;
  QTimer periodicCheckTimer, delayTimer;

  /* Need at least one megabyte to be valid */
  int minFileSize = 1024 * 1024;

  /* Delay event about two seconds to catch intermediate changes renamed files, etc. */
  int delayMs = 2000;

  /* Additionally check every ten seconds for changes */
  int checkMs = 10000;

  bool verbose;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_FILESYSTEMWATCHER_H
