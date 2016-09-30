/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

int FileResolver::getFiles(const SceneryArea& area, QStringList *filepaths, QStringList *filenames) const
{
  int numFiles = 0;
  QString sceneryAreaDirStr, areaLocalPathStr = area.getLocalPath();

  if(QFileInfo(areaLocalPathStr).isAbsolute())
    // Scenery local path is absolute - use it as is
    sceneryAreaDirStr = areaLocalPathStr;
  else
    // Scenery local path is relative - add base path
    sceneryAreaDirStr = options.getBasepath() + QDir::separator() + areaLocalPathStr;

  QFileInfo sceneryArea(sceneryAreaDirStr);
  if(sceneryArea.exists())
  {
    if(sceneryArea.isDir())
    {
      QDir sceneryAreaDir(sceneryArea.filePath());

      // get all scenery directories
      for(QFileInfo scenery : sceneryAreaDir.entryInfoList({"scenery"}, QDir::Dirs))
      {
        QDir sceneryAreaDirObj(scenery.filePath());
        // Check if directory is included
        if(options.isIncludedDirectory(sceneryAreaDirObj.absolutePath()))
        {
          // Get all BGL files
          for(QFileInfo bglFile : sceneryAreaDirObj.entryInfoList({"*.bgl"}, QDir::Files))
          {
            QString filename = bglFile.fileName();

            // Check if file is included
            if(options.isIncludedFilename(filename))
            {
              numFiles++;
              if(filepaths != nullptr)
                filepaths->append(bglFile.filePath());
              if(filenames != nullptr)
                filenames->append(bglFile.fileName());
            }
          }
        }
        else
          qInfo().nospace().noquote() << scenery.filePath() << " is excluded.";
      }
    }
    else if(!quiet)
      qWarning().nospace().noquote() << sceneryAreaDirStr << " is not a directory.";
  }
  else if(!quiet)
    qWarning().nospace().noquote() << "Directory " << sceneryAreaDirStr << " does not exist.";
  return numFiles;
}

} // namespace scenery
} // namespace fs
} // namespace atools
