/*****************************************************************************
* Copyright 2015-2022 Alexander Barthel alex@littlenavmap.org
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

#include <QDateTime>
#include <QSet>
#include <QTimer>
#include <QVector>

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
  virtual ~FileSystemWatcher() override;

  FileSystemWatcher(const FileSystemWatcher& other) = delete;
  FileSystemWatcher& operator=(const FileSystemWatcher& other) = delete;

  /* Set file and start watching. Does not emit an initial message. */
  void setFilenameAndStart(const QString& path);

  /* Set files and start watching. Does not emit an initial message.
   * All files have to be in the same folder which will be watched too. */
  void setFilenamesAndStart(const QStringList& pathList);

  /* Stop all notifications and watching */
  void stopWatching();

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
  /* Emitted once files are updated */
  void filesUpdated(const QStringList& filenames);

  /* Emitted once the parent folder of the files is updated */
  void dirUpdated(const QString& dirname);

private:
  void deleteFsWatcher();
  void createFsWatcher();

  /* Called on directory or file change and periodicCheckTimer event */
  void pathChanged();

  /* Called by delayTimer event */
  void pathUpdatedDelayed();

  void setPathsToFsWatcher(bool update);
  bool warn();

  /* Combines all information around a watched file or directory */
  struct PathInfo
  {
    PathInfo()
    {
    }

    PathInfo(const QString& pathParam, const QDateTime& timestampLastReadParam, qint64 sizeLastReadParam)
      : path(pathParam), timestampLastRead(timestampLastReadParam), sizeLastRead(std::move(sizeLastReadParam))
    {
    }

    QString path;
    QDateTime timestampLastRead;
    qint64 sizeLastRead = 0L;
  };

  friend QDebug operator<<(QDebug out, const PathInfo& record);

  // All watched files and their parent directory
  QVector<PathInfo> paths;

  // Indexes into above paths for pathUpdatedDelayed() filled by pathChanged() if changed
  QSet<int> changedPathIndexes;

  /* Calls pathChanged() on folder and file changes */
  QFileSystemWatcher *fsWatcher = nullptr;

  QTimer periodicCheckTimer, // Calls pathChanged()
         delayTimer; // pathUpdatedDelayed()

  /* Need at least one megabyte to be valid */
  int minFileSize = 1024 * 1024;

  /* Delay event about two seconds to catch intermediate changes, renamed files, etc. */
  int delayMs = 2000;

  /* Additionally check every ten seconds for changes to avoid lost events */
  int checkMs = 10000;

  bool verbose;

  // Limit maximum number of warning messages
  int numWarnings = 0;
  const static int MAX_WARNINGS = 20;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_FILESYSTEMWATCHER_H
