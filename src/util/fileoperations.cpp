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

bool FileOperations::copyDirectory(const QString& from, const QString& to, bool overwrite, bool hidden, bool system)
{
  filesProcessed = 0;
  errors.clear();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Current" << QDir().canonicalPath();

  // Check source folder
  errors.append(atools::checkDirMsg(from));

  // Check parent of destination folder
  QDir toDirParent(to);
  toDirParent.cdUp();
  errors.append(atools::checkDirMsg(toDirParent.absolutePath()));

  // Clear empty messages
  errors.removeAll(QString());

  if(hasErrors())
    return false;

  // Create destination folder if it does not exist
  if(!QFile::exists(to))
  {
    // Create only the top level - the parent has to exist already
    if(!QDir().mkdir(to))
    {
      errors.append(tr("Cannot create directory \"%1\".").arg(to));
      return false;
    }
  }

  // Copy recursively
  bool retval = copyDirectoryInternal(from, to, overwrite, hidden, system);

  if(hasErrors())
    qWarning().noquote().nospace() << Q_FUNC_INFO << "Errors: " << errors;

  return retval;
}

bool FileOperations::copyDirectoryInternal(const QString& from, const QString& to, bool overwrite, bool hidden, bool system)
{
  // Copy files first ================================================================
  QDir fromDir(from);
  QDir::Filters filter = QDir::Files;
  filter.setFlag(QDir::Hidden, hidden);
  filter.setFlag(QDir::System, system);
  const QStringList files = fromDir.entryList(filter);
  for(const QString& file : files)
  {
    // Remove existing file for overwrite
    QString toFile = to % QDir::separator() % file;
    if(overwrite && QFile::exists(toFile))
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "remove" << toFile;

      if(!QFile::remove(toFile))
      {
        errors.append(tr("Cannot remove file \"%1\".").arg(toFile));
        return false;
      }
    }

    QString fromFile = from % QDir::separator() % file;
    if(verbose)
      qDebug() << Q_FUNC_INFO << "copy" << fromFile << toFile;

    // Copy file
    if(!QFile::copy(fromFile, toFile))
    {
      errors.append(tr("Cannot copy file \"%1\" to \"%2\".").arg(fromFile).arg(toFile));
      return false;
    }
    else
      filesProcessed++;
  }

  // Create dirs next ================================================================
  QDir toDir(to);
  filter = QDir::Dirs | QDir::NoDotAndDotDot;
  filter.setFlag(QDir::Hidden, hidden);
  filter.setFlag(QDir::System, system);
  const QStringList dirs = fromDir.entryList(filter);
  for(const QString& dir : dirs)
  {
    QString toPath = to % QDir::separator() % dir;

    if(verbose)
      qDebug() << Q_FUNC_INFO << "mkdir" << toPath;

    // Create destination dir
    if(!toDir.exists(dir))
    {
      if(!toDir.mkdir(dir))
      {
        errors.append(tr("Cannot create directory \"%1\".").arg(toPath));
        return false;
      }
    }

    // Recurse
    if(!copyDirectoryInternal(from % QDir::separator() % dir, toPath, overwrite, hidden, system))
      return false;
  }

  return true;
}

bool FileOperations::removeDirectory(const QString& directory, bool keepDirs, bool hidden, bool system)
{
  filesProcessed = 0;
  errors.clear();

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Current" << QDir().canonicalPath();

  bool retval = false;
  if(!canRemoveDir(directory))
    // Removal not allowed since system folder
    errors.append(tr("Cannot remove standard directory, root drive or system folder \"%1\".").arg(directory));
  else
  {
    errors.append(atools::checkDirMsg(directory));
    errors.removeAll(QString());

    if(hasErrors())
      return false;

    retval = removeDirectoryInternal(directory, keepDirs, hidden, system);

    // Remove top level dir
    if(!keepDirs && !QDir().rmdir(directory))
      errors.append(tr("Cannot remove directory \"%1\".").arg(directory));

    if(hasErrors())
      qWarning().noquote().nospace() << Q_FUNC_INFO << "Errors: " << errors;
  }

  return retval;
}

bool FileOperations::removeDirectoryInternal(const QString& directory, bool keepDirs, bool hidden, bool system)
{
  QDir dir(directory);
  if(!dir.exists())
    return false;

  // Remove files first ======================================================================
  QDir::Filters filter = QDir::Files;
  filter.setFlag(QDir::Hidden, hidden);
  filter.setFlag(QDir::System, system);
  const QStringList files = dir.entryList(filter);
  for(const QString& file : files)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "remove" << file;

    if(!dir.remove(file))
      errors.append(tr("Cannot remove file \"%1\".").arg(dir.absoluteFilePath(file)));
    else
      filesProcessed++;
  }

  // Remove folders next ======================================================================
  filter = QDir::Dirs | QDir::NoDotAndDotDot;
  filter.setFlag(QDir::Hidden, hidden);
  filter.setFlag(QDir::System, system);
  const QStringList dirs = dir.entryList(filter);
  for(const QString& d : dirs)
  {
    // Recurse
    removeDirectoryInternal(dir.absoluteFilePath(d), keepDirs, hidden, system);
    if(!keepDirs && !dir.rmdir(d))
      errors.append(tr("Cannot remove directory \"%1\".").arg(dir.absoluteFilePath(d)));
  }

  return true;
}

bool FileOperations::canRemoveDir(const QString& dir) const
{
  QDir d(dir);

  return !allDefaultPaths.contains(atools::canonicalFilePath(d.absolutePath()), Qt::CaseInsensitive) &&
         !d.isRoot() && dir != "." && dir != ".." && dir != QDir::homePath() && dir != QDir::tempPath();
}

} // namespace util
} // namespace atools
