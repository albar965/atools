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
#include "fs/bglreaderoptions.h"

#include <QtDebug>
#include <QFile>
#include <QDir>

namespace atools {
namespace fs {
namespace scenery {

FileResolver::FileResolver(const BglReaderOptions& opts, bool noWarnings)
  : options(opts), quiet(noWarnings)
{
}

FileResolver::~FileResolver()
{
}

FileResolver& FileResolver::addExcludedFilePrefixes(const QStringList& prefixes)
{
  excludedPrefixes += prefixes;
  return *this;
}

void FileResolver::clearExcludedFilePrefixes()
{
  excludedPrefixes.clear();
}

int FileResolver::getFiles(const SceneryArea& area, QStringList *filepaths, QStringList *filenames) const
{
  int numFiles = 0;
  QString sceneryAreaDirStr, areaLocalPathStr = area.getLocalPath();

  if(QFileInfo(areaLocalPathStr).isAbsolute())
    sceneryAreaDirStr = areaLocalPathStr;
  else
    sceneryAreaDirStr = options.getBasepath() + QDir::separator() + areaLocalPathStr;

  QFileInfo sceneryArea(sceneryAreaDirStr);
  if(sceneryArea.exists())
  {
    if(sceneryArea.isDir())
    {
      QDir sceneryAreaDir(sceneryArea.filePath());
      for(QFileInfo scenery : sceneryAreaDir.entryInfoList({"scenery"}, QDir::Dirs))
      {
        QDir sceneryAreaDirObj(scenery.filePath());
        for(QFileInfo bglFile : sceneryAreaDirObj.entryInfoList({"*.bgl"}, QDir::Files))
        {
          QString filename = bglFile.fileName();
          if(!matchesExcludedPrefix(filename))
            if(options.includeFilename(filename))
            {
              numFiles++;
              if(filepaths != nullptr)
                filepaths->push_back(bglFile.filePath());
              if(filenames != nullptr)
                filenames->push_back(bglFile.fileName());
            }
        }
      }
    }
    else if(!quiet)
      qWarning().nospace().noquote() << sceneryAreaDirStr << " is not a directory.";
  }
  else if(!quiet)
    qWarning().nospace().noquote() << "Directory " << sceneryAreaDirStr << " does not exist.";
  return numFiles;
}

bool FileResolver::matchesExcludedPrefix(const QString& fname) const
{
  for(QString i : excludedPrefixes)
    if(fname.startsWith(i, Qt::CaseInsensitive))
      return true;

  return false;
}

} // namespace scenery
} // namespace fs
} // namespace atools
