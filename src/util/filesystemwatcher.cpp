/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "util/filesystemwatcher.h"

#include "atools.h"

#include <QFileInfo>
#include <QFileSystemWatcher>

namespace atools {
namespace util {

QDebug operator<<(QDebug out, const FileSystemWatcher::PathInfo& record)
{
  out.nospace() << "PathInfo[" << record.path << ", " << record.timestampLastRead << ", " << record.sizeLastRead << "]";
  return out;
}

FileSystemWatcher::FileSystemWatcher(QObject *parent, bool verboseLogging)
  : QObject(parent), verbose(verboseLogging)
{
  qDebug() << Q_FUNC_INFO;

  delayTimer.setSingleShot(true);
  QTimer::connect(&delayTimer, &QTimer::timeout, this, &FileSystemWatcher::pathUpdatedDelayed);
}

FileSystemWatcher::~FileSystemWatcher()
{
  qDebug() << Q_FUNC_INFO;

  stopWatching();
}

void FileSystemWatcher::stopWatching()
{
  deleteFsWatcher();
  paths.clear();
  changedPathIndexes.clear();
}

void FileSystemWatcher::pathChanged()
{
  if(verbose)
  {
    qDebug() << Q_FUNC_INFO << "directories" << fsWatcher->directories();
    qDebug() << Q_FUNC_INFO << "files" << fsWatcher->files();
  }

  // Stop all timers - one of the other will be started later
  periodicCheckTimer.stop();
  delayTimer.stop();

  for(int i = 0; i < paths.size(); i++)
  {
    const PathInfo& info = paths.at(i);

    QFileInfo fileinfo(info.path);
    if(fileinfo.exists())
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "Path" << info.path
                 << "file" << fileinfo.isFile()
                 << "exists" << fileinfo.exists()
                 << "size" << fileinfo.size()
                 << "last modified" << fileinfo.lastModified().toString(Qt::ISODateWithMs);

      if(fileinfo.isFile())
      {
        if(fileinfo.size() > minFileSize)
        {
          // File exists - first call or older than one second or file differs
          if(!info.timestampLastRead.isValid() || std::abs(fileinfo.lastModified().msecsTo(info.timestampLastRead)) > 1000L ||
             info.sizeLastRead != fileinfo.size())
          {
            // Timestamp of file has changed
            if(verbose)
              qDebug() << Q_FUNC_INFO << "=== File changed" << info.path;

            // Start or extend the delayed notification
            changedPathIndexes.insert(i);
          }
          else
          {
            if(verbose)
              qDebug() << Q_FUNC_INFO << "File not changed" << info.path;
          }
        } // if(fileinfo.size() > minFileSize)
        else
        {
          // File is being updated - keep current file
          if(warn())
            qWarning() << Q_FUNC_INFO << "File" << info.path << "smaller than" << minFileSize << "bytes";
        }
      } // if(fileinfo.isFile())
      else if(fileinfo.isDir())
      {
        // Notification on dir change - keep current file
        if(!info.timestampLastRead.isValid() || std::abs(fileinfo.lastModified().msecsTo(info.timestampLastRead)) > 1000L)
        {
          if(verbose)
            qDebug() << Q_FUNC_INFO << "=== Dir changed" << info.path;
          // Start or extend the delayed notification
          changedPathIndexes.insert(i);
        }
        else
        {
          if(verbose)
            qDebug() << Q_FUNC_INFO << "Dir not changed" << info.path;
        }
      }
    } // if(fileinfo.exists())
    else
    {
      // File was deleted - keep current file and do not send a notification
      if(warn())
        qWarning() << Q_FUNC_INFO << "File" << info.path << "does not exist";
    }
  } // for(int i = 0; i < paths.size(); i++)

  if(!changedPathIndexes.isEmpty())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "Update - starting delayed update";

    // Start or extend the delayed notification to pathUpdatedDelayed()
    delayTimer.start(delayMs);
  }
  else
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "No update - starting timer";

    // Start timer to check periodically pathChanged()
    periodicCheckTimer.start(checkMs);
  }

  setPathsToFsWatcher(true);
}

void FileSystemWatcher::pathUpdatedDelayed()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << changedPathIndexes;

  // Collect existing changed files and the parent directory if changed
  QStringList updatedFiles, updatedDirs;
  for(int index : std::as_const(changedPathIndexes))
  {
    if(!atools::inRange(paths, index))
    {
      if(warn())
        qWarning() << Q_FUNC_INFO << "invalid index" << index << "size" << paths.size();
      continue;
    }

    PathInfo& info = paths[index];

    QFileInfo fileinfo(info.path);
    if(fileinfo.exists())
    {
      // Either file with sufficient size of directory
      if((fileinfo.isFile() && fileinfo.size() > minFileSize) || fileinfo.isDir())
      {
        // Remember file size and timestamp of the file
        info.timestampLastRead = fileinfo.lastModified();
        info.sizeLastRead = fileinfo.size();

        if(fileinfo.isDir())
          updatedDirs.append(info.path);
        else
          updatedFiles.append(info.path);
      }
    }
    else
    {
      // File was deleted
      if(warn())
        qWarning() << Q_FUNC_INFO << "File" << info.path << "does not exist";
    }
  }
  changedPathIndexes.clear();

  if(!updatedDirs.isEmpty())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "Updated dirs" << updatedDirs;
    emit dirUpdated(updatedDirs.constFirst());
  }

  if(!updatedFiles.isEmpty())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "Updated files" << updatedFiles;

    emit filesUpdated(updatedFiles);
  }

  // pathChanged()
  periodicCheckTimer.start(checkMs);
}

void FileSystemWatcher::setFilenameAndStart(const QString& path)
{
  setFilenamesAndStart({path});
}

void FileSystemWatcher::setFilenamesAndStart(const QStringList& pathList)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << pathList;

  stopWatching();

  // Get all files
  for(const QString& path : pathList)
  {
    QFileInfo fileinfo(path);
    if(fileinfo.isFile())
      paths.append(PathInfo(path, fileinfo.lastModified(), fileinfo.size()));
  }

  // Get parent folder of first file or directory
  if(!pathList.isEmpty())
  {
    QFileInfo fileinfo(pathList.constFirst());
    QString path;
    if(fileinfo.isFile())
      path = fileinfo.path();
    else if(fileinfo.isDir())
      path = fileinfo.filePath();

    paths.append(PathInfo(path, fileinfo.lastModified(), fileinfo.size()));
  }

  createFsWatcher();
}

void FileSystemWatcher::deleteFsWatcher()
{
  delayTimer.stop();
  periodicCheckTimer.stop();

  QTimer::disconnect(&periodicCheckTimer, &QTimer::timeout, this, &FileSystemWatcher::pathChanged);

  if(fsWatcher != nullptr)
  {
    QFileSystemWatcher::disconnect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &FileSystemWatcher::pathChanged);
    QFileSystemWatcher::disconnect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &FileSystemWatcher::pathChanged);
    fsWatcher->deleteLater();
    fsWatcher = nullptr;
  }
}

void FileSystemWatcher::createFsWatcher()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << paths;

  if(fsWatcher == nullptr)
  {
    // Watch file for changes and directory too to catch file deletions
    fsWatcher = new QFileSystemWatcher(this);
    QFileSystemWatcher::connect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &FileSystemWatcher::pathChanged);
    QFileSystemWatcher::connect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &FileSystemWatcher::pathChanged);
  }

  setPathsToFsWatcher(false);

  // Initialize size and timestamp which will omit the first update signal - user has to do the initial load
  for(PathInfo& info : paths)
  {
    QFileInfo fileinfo(info.path);
    if(fileinfo.exists())
    {
      info.timestampLastRead = fileinfo.lastModified();
      info.sizeLastRead = fileinfo.size();
    }
  }

  // Check every ten seconds since the watcher is unreliable
  QTimer::connect(&periodicCheckTimer, &QTimer::timeout, this, &FileSystemWatcher::pathChanged);
  periodicCheckTimer.start(checkMs);
}

void FileSystemWatcher::setPathsToFsWatcher(bool update)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << paths << "files" << fsWatcher->files() << "dirs" << fsWatcher->directories();

  if(fsWatcher == nullptr)
    return;

  QStringList files = fsWatcher->files();
  QStringList directories = fsWatcher->directories();
  for(const PathInfo& info : std::as_const(paths))
  {
    // Watch file to get changes
    if(!files.contains(info.path) && !directories.contains(info.path))
    {
      if(update && verbose && warn())
        qDebug() << "dropped path" << info.path << files << directories;

      if(QFileInfo::exists(info.path))
      {
        if(!fsWatcher->addPath(info.path))
        {
          if(warn())
            qWarning() << "cannot watch file" << info.path;
        }
      }
      else
        fsWatcher->removePath(info.path);
    }
  }

  if(verbose)
  {
    qDebug() << Q_FUNC_INFO << "directories" << fsWatcher->directories();
    qDebug() << Q_FUNC_INFO << "files" << fsWatcher->files();
  }
}

bool FileSystemWatcher::warn()
{
  if(numWarnings < MAX_WARNINGS)
  {
    numWarnings++;
    return true;
  }
  else if(numWarnings++ == MAX_WARNINGS)
    qWarning() << Q_FUNC_INFO << "Maximum number of warnings exceeded";
  return false;
}

} // namespace util
} // namespace atools
