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

#include "util/fileoperations.h"
#include "atools.h"

#include <QDir>
#include <QStringBuilder>
#include <QDebug>
#include <QStandardPaths>

namespace atools {
namespace util {

FileOperations::FileOperations(bool verboseParam)
  : verbose(verboseParam)
{
  // Collect standard paths and refuse to delete these
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::FontsLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::MusicLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::TempLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::HomeLocation));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::DataLocation));
#endif
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::CacheLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation));
  allDefaultPaths.append(QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation));

  // Refuse to delete root drives
  const QFileInfoList drives = QDir::drives();
  for(const QFileInfo& drive : drives)
  {
    allDefaultPaths.append(drive.absoluteFilePath());

    // No Windows system folders
#ifdef Q_OS_WIN
    allDefaultPaths.append(drive.absoluteFilePath() % QDir::separator() % "Windows");
    allDefaultPaths.append(drive.absoluteFilePath() % QDir::separator() % "Windows" % QDir::separator() % "System32");
#endif
  }

  // Convert all to canonical
  for(QString& path : allDefaultPaths)
    path = atools::canonicalFilePath(path);
}

void FileOperations::copyDirectory(const QString& from, const QString& to, bool overwrite, bool hidden, bool system)
{
  filesProcessed = 0;
  errors.clear();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Current" << QDir().canonicalPath();

  // Check source folder
  errors.append(atools::checkDirMsg(from));
  errors.removeAll(QString());

  if(!hasErrors())
  {
    // Check parent of destination folder
    QDir toDirParent(to);
    toDirParent.cdUp();
    errors.append(atools::checkDirMsg(toDirParent.absolutePath()));
    errors.removeAll(QString());

    if(!hasErrors())
    {
      // Create destination folder if it does not exist
      if(!QFile::exists(to))
      {
        // Create only the top level - the parent has to exist already
        if(!QDir().mkdir(to))
          errors.append(tr("Cannot create directory \"%1\".").arg(to));
      }

      // Copy recursively
      if(!hasErrors())
        copyDirectoryInternal(from, to, overwrite, hidden, system);
    }
  }

  if(hasErrors())
    qWarning().noquote().nospace() << Q_FUNC_INFO << "Errors: " << errors;
}

void FileOperations::copyDirectoryInternal(const QString& from, const QString& to, bool overwrite, bool hidden, bool system)
{
  // Set up filter
  QDir::Filters filter = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
  QDir fromDir(from);
  filter.setFlag(QDir::Hidden, hidden);
  filter.setFlag(QDir::System, system);
  const QFileInfoList infoList = fromDir.entryInfoList(filter, QDir::DirsFirst); // Recurse first into directories

  for(const QFileInfo& fromPath : infoList)
  {
    QFileInfo toPath(to % QDir::separator() % fromPath.fileName());

    if(overwrite && toPath.exists() && (toPath.isFile() || toPath.isSymbolicLink() || toPath.isJunction() || toPath.isShortcut()))
    {
      // Remove existing file or link for overwrite ====================================
      if(verbose)
        qDebug() << Q_FUNC_INFO << "remove" << toPath.filePath();

      if(!QFile::remove(toPath.filePath()))
        errors.append(tr("Cannot remove file \"%1\".").arg(toPath.filePath()));
    }

    if(!hasErrors())
    {
      if(fromPath.isSymbolicLink())
      {
        // Create a symbolic link - needs relative links ======================================================
        QString relativeLinkTarget = QDir(fromPath.absolutePath()).relativeFilePath(atools::linkTarget(fromPath));

        if(verbose)
          qDebug() << Q_FUNC_INFO << "link from" << toPath.filePath() << "to" << relativeLinkTarget;

        if(!QFile::link(relativeLinkTarget, toPath.filePath()))
          errors.append(tr("Cannot create link \"%1\" to \"%2\".").arg(toPath.absoluteFilePath()).arg(relativeLinkTarget));
        else
          filesProcessed++;
      }
      else
      {
        if(fromPath.isFile())
        {
          // Copy file ================================================================
          if(verbose)
            qDebug() << Q_FUNC_INFO << "copy from" << fromPath.filePath() << "to" << toPath.filePath();

          if(!QFile::copy(fromPath.filePath(), toPath.filePath()))
            errors.append(tr("Cannot copy file \"%1\" to \"%2\".").arg(fromPath.filePath()).arg(toPath.filePath()));
          else
            filesProcessed++;
        }
        else if(fromPath.isDir())
        {
          // Create dir ================================================================
          if(verbose)
            qDebug() << Q_FUNC_INFO << "mkdir" << toPath.filePath();

          // Create destination dir
          if(!toPath.exists())
          {
            if(!toPath.dir().mkdir(toPath.fileName()))
              errors.append(tr("Cannot create directory \"%1\".").arg(toPath.absoluteFilePath()));
          }

          // Recurse copying ==================================================
          if(!hasErrors())
            copyDirectoryInternal(fromPath.filePath(), toPath.filePath(), overwrite, hidden, system);
        }
      }
    }
  }
}

void FileOperations::removeDirectory(const QString& directory, bool keepDirs, bool hidden, bool system)
{
  filesProcessed = 0;
  errors.clear();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Current" << QDir().canonicalPath();

  if(!canRemoveDir(directory))
    // Removal not allowed since system folder
    errors.append(tr("Cannot remove standard directory, root drive or system folder \"%1\".").arg(directory));
  else
  {
    errors.append(atools::checkDirMsg(directory));
    errors.removeAll(QString());

    if(!hasErrors())
    {
      removeDirectoryInternal(directory, keepDirs, hidden, system);

      if(!hasErrors())
      {
        // Remove top level dir
        if(!keepDirs && !QDir().rmdir(directory))
          errors.append(tr("Cannot remove directory \"%1\".").arg(directory));
      }
    }
  }

  if(hasErrors())
    qWarning().noquote().nospace() << Q_FUNC_INFO << "Errors: " << errors;
}

void FileOperations::removeDirectoryToTrash(const QString& directory)
{
  filesProcessed = 0;
  errors.clear();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Current" << QDir().canonicalPath();

  errors.append(atools::checkDirMsg(directory));
  errors.removeAll(QString());

  if(!hasErrors())
  {
    QString trashName;
    if(!QFile::moveToTrash(directory, &trashName))
      errors.append(tr("Cannot move directory \"%1\" to trash.").arg(directory));
    else if(verbose)
    {
      filesProcessed++;
      if(verbose)
        qDebug() << Q_FUNC_INFO << directory << "moved to" << trashName;
    }
  }

  if(hasErrors())
    qWarning().noquote().nospace() << Q_FUNC_INFO << "Errors: " << errors;
}

void FileOperations::removeDirectoryInternal(const QString& directory, bool keepDirs, bool hidden, bool system)
{
  QDir dir(directory);
  if(dir.exists())
  {
    // Dive into subdirs and then remove files ======================================================================
    QDir::Filters filter = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    filter.setFlag(QDir::Hidden, hidden);
    filter.setFlag(QDir::System, system);
    const QFileInfoList infoList = dir.entryInfoList(filter, QDir::DirsFirst);
    for(const QFileInfo& fileinfo : infoList)
    {
      QString absFilePath = fileinfo.absoluteFilePath();

      if(fileinfo.isDir() && !fileinfo.isSymbolicLink())
      {
        // Recurse but not into symbolic links
        removeDirectoryInternal(absFilePath, keepDirs, hidden, system);
        if(!keepDirs)
        {
          if(verbose)
            qDebug() << Q_FUNC_INFO << "remove dir" << absFilePath;

          if(!dir.rmdir(absFilePath))
            errors.append(tr("Cannot remove directory \"%1\".").arg(absFilePath));
        }
      }
      else if(fileinfo.isFile() || fileinfo.isSymbolicLink() || fileinfo.isJunction() || fileinfo.isShortcut())
      {
        if(verbose)
          qDebug() << Q_FUNC_INFO << "remove file" << absFilePath;

        if(!QFile::remove(absFilePath))
          errors.append(tr("Cannot remove \"%1\".").arg(absFilePath));
        else
          filesProcessed++;
      }
    }
  }
  else
    errors.append(tr("Directory does not exist \"%1\".").arg(directory));
}

bool FileOperations::canRemoveDir(const QString& dir) const
{
  QDir d(dir);

  return !allDefaultPaths.contains(atools::canonicalFilePath(d.absolutePath()), Qt::CaseInsensitive) &&
         !d.isRoot() && dir != "." && dir != ".." && dir != QDir::homePath() && dir != QDir::tempPath();
}

} // namespace util
} // namespace atools
