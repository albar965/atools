/*
 * FileResolver.cpp
 *
 *  Created on: 28.04.2015
 *      Author: alex
 */

#include "fileresolver.h"
#include "sceneryarea.h"
#include "../bglreaderoptions.h"

#include <QtDebug>
#include <QFile>
#include <QDir>

namespace atools {
namespace fs {
namespace scenery {

FileResolver::FileResolver(const BglReaderOptions& opts)
  : options(opts)
{
}

FileResolver::~FileResolver()
{
}

void FileResolver::getFiles(const SceneryArea& area, QStringList& files) const
{
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
      QFileInfo sceneryAreaDir(sceneryArea.absoluteFilePath() + QDir::separator() + "scenery");
      if(sceneryAreaDir.exists() && sceneryAreaDir.isDir())
      {
        QDir sceneryAreaDirObj(sceneryAreaDir.absoluteFilePath());
        for(QFileInfo bglFile : sceneryAreaDirObj.entryInfoList({"*.bgl"}, QDir::Files))
          if(!matchesExcludedPrefix(bglFile.fileName()))
            files.push_back(bglFile.absolutePath());
      }
    }
    else
      qWarning().nospace().noquote() << sceneryAreaDirStr << " is not a directory.";
  }
  else
    qWarning().nospace().noquote() << "Directory " << sceneryAreaDirStr << " does not exist.";
}

bool FileResolver::matchesExcludedPrefix(const QString& fname) const
{
  for(QString i : excludedPrefixes)
    if(fname.startsWith(i))
      return true;

  return false;
}

} // namespace scenery
} // namespace fs
} // namespace atools
