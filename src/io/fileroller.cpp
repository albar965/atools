/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include <QFile>
#include <QStringList>

namespace atools {
namespace io {

FileRoller::FileRoller(int maxNumFiles)
  : maxFiles(maxNumFiles)
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
    QFile oldFile(filename + "." + QString::number(i));
    QFile newFile(filename + "." + QString::number(i + 1));

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
    renameSafe(filename, filename + ".1");
}

void FileRoller::renameSafe(const QString& oldFile, const QString& newFile)
{
  if(QFile::exists(newFile))
    QFile(newFile).remove();
  QFile(oldFile).rename(newFile);
}

} /* namespace io */
} // namespace atools
