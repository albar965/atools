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

#include "io/fileroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace atools {
namespace io {

FileRoller::FileRoller(int maxNumFiles, const QString& filePattern, bool keepOriginalFileParam)
  : maxFiles(maxNumFiles), keepOriginalFile(keepOriginalFileParam), pattern(filePattern)
{

}

void FileRoller::rollFiles(const QStringList& filenames)
{
  for(const QString& fn : filenames)
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
        renameSafe(oldFile.fileName(), newFile.fileName(), false /* originalFile */);
    }
  }

  if(maxFiles > 0)
    renameSafe(filename, buildFilename(filename, 1), true /* originalFile */);
}

QString FileRoller::buildFilename(const QString& filename, int num) const
{
  // ${base}: Complete basename, ${num}: Counting number, ${ext}: File extension.
  const static QLatin1String DEFAULT_PATTERN("${base}.${ext}.${num}");

  QFileInfo fileinfo(filename);
  return fileinfo.path() + QDir::separator() + QString(pattern.isEmpty() ? DEFAULT_PATTERN : pattern).
         replace("${base}", fileinfo.completeBaseName()).
         replace("${num}", QString::number(num)).
         replace("${ext}", fileinfo.suffix());
}

void FileRoller::renameSafe(const QString& fromFile, const QString& toFile, bool originalFile) const
{
  if(QFile::exists(toFile))
    QFile(toFile).remove();

  if(keepOriginalFile && originalFile)
    QFile(fromFile).copy(toFile);
  else
    QFile(fromFile).rename(toFile);
}

} /* namespace io */
} // namespace atools
