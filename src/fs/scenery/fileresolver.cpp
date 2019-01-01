/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "fs/scenery/fileresolver.h"
#include "fs/scenery/sceneryarea.h"
#include "fs/navdatabaseoptions.h"

#include <QtDebug>
#include <QFile>
#include <QDir>

namespace atools {
namespace fs {
namespace scenery {

FileResolver::FileResolver(const NavDatabaseOptions& opts, bool noWarnings)
  : options(opts), quiet(noWarnings)
{
}

FileResolver::~FileResolver()
{
}

int FileResolver::getFiles(const SceneryArea& area, QStringList *filepaths, QStringList *filenames)
{
  int numFiles = 0;
  errorMessages.clear();

  QString sceneryAreaDirStr, areaLocalPathStr = area.getLocalPath();

  if(QFileInfo(areaLocalPathStr).isAbsolute())
    // Scenery local path is absolute - use it as is
    sceneryAreaDirStr = areaLocalPathStr;
  else
    // Scenery local path is relative - add base path
    sceneryAreaDirStr = options.getBasepath() + QDir::separator() + areaLocalPathStr;

  // Remove any .. in the path but do not change symlinks
  QString sceneryAreaDirStrFilePath = QFileInfo(sceneryAreaDirStr).absoluteFilePath();

  qInfo() << "Scenery path" << sceneryAreaDirStr;

  QFileInfo sceneryArea(sceneryAreaDirStrFilePath);
  if(sceneryArea.exists())
  {
    if(sceneryArea.isDir())
    {
      QDir sceneryAreaDir(sceneryArea.filePath());

      QFileInfoList sceneryDirs;
      sceneryDirs.append(sceneryAreaDir.entryInfoList({"scenery"},
                                                      QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot));

      if(sceneryDirs.isEmpty() && sceneryAreaDir.dirName().toLower() == "scenery")
        // Special case where entry points to scenery directory which is allowed by P3D
        sceneryDirs.append(QFileInfo(sceneryAreaDir.path()));

      // get all scenery directories - normally only one
      for(QFileInfo scenery : sceneryDirs)
      {
        if(scenery.isDir())
        {
          QDir sceneryAreaDirObj(scenery.filePath());
          // Check if directory is included
          if(options.isIncludedDirectory(sceneryAreaDirObj.absolutePath()))
          {
            // Get all BGL files
            for(QFileInfo bglFile : sceneryAreaDirObj.entryInfoList(
                  {"*.bgl"}, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                  QDir::Name | QDir::IgnoreCase))
            {
              if(bglFile.isFile() && bglFile.isReadable())
              {
                QString filename = bglFile.fileName();
                QString filepath = bglFile.filePath();

                // Check if file is included from config file and GUI options
                if(options.isIncludedFilename(filename) && options.isIncludedFilePath(filepath))
                {
                  numFiles++;
                  if(filepaths != nullptr)
                    filepaths->append(filepath);
                  if(filenames != nullptr)
                    filenames->append(filename);
                }
              }
              else
                qWarning().nospace().noquote() << scenery.filePath() << " is no file or not readable.";
            }
          }
          else
            qInfo().nospace().noquote() << scenery.filePath() << " is excluded.";
        }
        else
          qWarning().nospace().noquote() << scenery.filePath() << " is no directory.";
      }
    }
    else
    {
      if(!quiet)
        qWarning().nospace().noquote() << sceneryAreaDirStr << " is not a directory.";
      errorMessages.append(tr("\"%2\" is not a directory.").arg(sceneryAreaDirStr));
    }
  }
  else
  {
    if(!quiet)
      qWarning().nospace().noquote() << "Directory " << sceneryAreaDirStr << " does not exist.";
    errorMessages.append(tr("\"%2\" does not exist.").arg(sceneryAreaDirStr));
  }
  return numFiles;
}

} // namespace scenery
} // namespace fs
} // namespace atools
