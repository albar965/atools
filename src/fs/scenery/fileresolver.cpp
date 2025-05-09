/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "atools.h"
#include "fs/navdatabaseoptions.h"
#include "fs/scenery/fileresolver.h"
#include "fs/scenery/layoutjson.h"
#include "fs/scenery/sceneryarea.h"

#include <QtDebug>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

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
  if((!area.isActive() && !options.isReadInactive()) || !options.isIncludedLocalPath(area.getLocalPath()))
    return 0;

  int numFiles = 0;
  errorMessages.clear();

  QString sceneryAreaDirStr, areaLocalPathStr = area.getLocalPath();

  if(QFileInfo(areaLocalPathStr).isAbsolute())
    // Scenery local path is absolute - use it as is
    sceneryAreaDirStr = areaLocalPathStr;
  else
  {
    // Scenery local path is relative - add base path
    if(options.getSimulatorType() == atools::fs::FsPaths::MSFS || options.getSimulatorType() == atools::fs::FsPaths::MSFS_2024)
    {
      // Base is C:\Users\alex\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages

      if(area.isIncluded())
        // Include from GUI has full path
        sceneryAreaDirStr = areaLocalPathStr;
      else if(area.isCommunity())
        // C:\Users\alex\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages\Community\ADDON
        sceneryAreaDirStr = options.getMsfsCommunityPath() + SEP + areaLocalPathStr;
      else
      {
        if(options.getSimulatorType() == atools::fs::FsPaths::MSFS)
          // C:\Users\alex\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages\Official\OneStore
          sceneryAreaDirStr = options.getMsfsOfficialPath() + SEP + areaLocalPathStr;
      }
    }
    else
      // FSX or P3D
      sceneryAreaDirStr = options.getBasepath() + SEP + areaLocalPathStr;
  }

  // Remove any .. in the path but do not change symlinks
  qInfo() << "Scenery path" << sceneryAreaDirStr;

  QFileInfo sceneryArea(QFileInfo(sceneryAreaDirStr).absoluteFilePath());
  if(sceneryArea.exists())
  {
    if(sceneryArea.isDir())
    {
      QFileInfoList sceneryDirs;
      if(options.getSimulatorType() == atools::fs::FsPaths::MSFS || options.getSimulatorType() == atools::fs::FsPaths::MSFS_2024)
        // MSFS has one scenery folder with layout.json
        sceneryDirs.append(sceneryArea);
      else
      {
        // Get all scenery folders for FSX and P3D
        QDir sceneryAreaDir(sceneryArea.filePath());

        sceneryDirs.append(sceneryAreaDir.entryInfoList({"scenery"}, QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot));

        if(sceneryDirs.isEmpty() && sceneryAreaDir.dirName().toLower() == "scenery")
          // Special case where entry points to scenery directory which is allowed by P3D
          sceneryDirs.append(QFileInfo(sceneryAreaDir.path()));
      }

      scenery::LayoutJson layout;

      // get all scenery directories - normally only one
      for(const QFileInfo& scenery : sceneryDirs)
      {
        if(scenery.isDir())
        {
          // Check if directory is included
          if(options.isIncludedGui(scenery))
          {
            QFileInfoList bglFiles;

            if(options.getSimulatorType() == atools::fs::FsPaths::MSFS)
            {
              // Read MSFS layout file and add all BGL files ================
              layout.clear();
              layout.read(scenery.absoluteFilePath() + SEP + "layout.json");

              for(const QString& path : layout.getBglPaths())
              {
                // Fix paths with wrong case for testing on Linux
                const static QRegularExpression SEPREGEXP("[\\/]");
                QStringList pathList;
#ifdef Q_OS_LINUX
                pathList.append("/");
#endif
                pathList.append(sceneryArea.filePath().split(SEPREGEXP));
                pathList.append(path.split(SEPREGEXP));
                bglFiles.append(atools::buildPathNoCase(pathList));
              }
            }
            else if(options.getSimulatorType() == atools::fs::FsPaths::MSFS_2024)
            {
              QDir sceneryAreaDirObj(scenery.absoluteFilePath());
              QFileInfoList dirs = sceneryAreaDirObj.entryInfoList(QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                                                                   QDir::Name | QDir::IgnoreCase);
              for(const QFileInfo& dir : qAsConst(dirs))
              {
                QDir dirObj(dir.absoluteFilePath());
                bglFiles.append(dirObj.entryInfoList({"*.bgl"},
                                                     QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                                                     QDir::Name | QDir::IgnoreCase));

              }
            }
            else
            {
              // Read all BGL files from directory structure ==============
              QDir sceneryAreaDirObj(scenery.absoluteFilePath());
              bglFiles = sceneryAreaDirObj.entryInfoList({"*.bgl"},
                                                         QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                                                         QDir::Name | QDir::IgnoreCase);
            }

            // Get all BGL files
            for(const QFileInfo& bglFile : qAsConst(bglFiles))
            {
              if(bglFile.isFile() && bglFile.isReadable())
              {
                QString filename = bglFile.fileName();
                QString filepath = bglFile.filePath();

                // Check if file is included from config file and GUI options
                if(options.isIncludedFilename(filename) && options.isIncludedGui(bglFile))
                {
                  // Skip maintenance BGL from Navigraph udpate in MSFS
                  if(options.getSimulatorType() == atools::fs::FsPaths::MSFS &&
                     area.isMsfsNavigraphNavdata() && filename.compare("maintenance.bgl", Qt::CaseInsensitive) == 0)
                    continue;

                  numFiles++;
                  if(filepaths != nullptr)
                    filepaths->append(filepath);
                  if(filenames != nullptr)
                    filenames->append(filename);
                }
              }
#ifndef DEBUG_SILENCE_COMPILER_WARNINGS
              else
                qWarning().nospace().noquote() << bglFile.absoluteFilePath() << " is no file or not readable.";
#endif
            }
          }
          else
            qInfo().nospace().noquote() << scenery.filePath() << " is excluded.";
        }
        else
          qWarning().nospace().noquote() << scenery.filePath() << " is not a directory.";
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
