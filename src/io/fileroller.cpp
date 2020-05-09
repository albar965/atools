/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "io/fileroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace atools {
namespace io {

FileRoller::FileRoller(int maxNumFiles)
  : maxFiles(maxNumFiles)
{

}

FileRoller::FileRoller(int maxNumFiles, const QString& filePattern)
  : maxFiles(maxNumFiles), pattern(filePattern)
{

}

void FileRoller::rollFiles(const QStringList& filenames)
{
  for(QString fn : filenames)
    rollFile(fn);
}

void FileRoller::rollFile(const QString& filename)
{
  for(int i = maxFiles; i >= 1; --i)
  {
    QFile oldFile(buildFilename(filename, i));
    QFile newFile(buildFilename(filename, i + 1));

    if(oldFile.exists())
    {
      if(i == maxFiles)
        // Remove oldest
        oldFile.remove();
      else
        // Move all other to higher number
        renameSafe(oldFile.fileName(), newFile.fileName());
    }
  }

  if(maxFiles > 0)
    renameSafe(filename, buildFilename(filename, 1));
}

QString FileRoller::buildFilename(const QString& filename, int num) const
{
  QFileInfo fileinfo(filename);
  return fileinfo.path() + QDir::separator() +
         QString(pattern).replace("${base}", fileinfo.completeBaseName()).
         replace("${num}", QString::number(num)).
         replace("${ext}", fileinfo.suffix());
}

void FileRoller::renameSafe(const QString& oldFile, const QString& newFile) const
{
  if(QFile::exists(newFile))
    QFile(newFile).remove();
  QFile(oldFile).rename(newFile);
}

} /* namespace io */
} // namespace atools
