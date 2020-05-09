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

#ifndef ATOOLS_IO_FILEROLLER_H
#define ATOOLS_IO_FILEROLLER_H

#include <QString>

class QString;
class QStringList;

namespace atools {
namespace io {

/*
 * Creates numbered backups from e.g. log files.
 */
class FileRoller
{
public:
  /*
   *  @param maxNumFiles Maximum number of backups to keep.
   */
  FileRoller(int maxNumFiles);
  /*
   *  @param maxNumFiles Maximum number of backups to keep.
   *  @parm filePattern
   *     ${base}: Complete basename.
   *     ${sep}: fileSeparator.
   *     ${num}: Counting number.
   *     ${ext}: File extension.
   *     Directory is always prepended.
   */
  FileRoller(int maxNumFiles, const QString& filePattern);

  /*
   * Create numbered backups of a file. Maximum number of files results in:
   * file.log, file.log.1, ... , file.log.maxFiles
   *
   * @param Filename to backup
   */
  void rollFile(const QString& filename);

  /*
   * Roll a list of files.
   * @param filenames List of files.
   */
  void rollFiles(const QStringList& filenames);

private:
  void renameSafe(const QString& oldFile, const QString& newFile) const;
  QString buildFilename(const QString& filename, int num) const;

  int maxFiles = 0;

  // "${base}${sep}${num}.${ext}"
  QString pattern = "${base}.${ext}.${num}";
};

} /* namespace io */
} // namespace atools

#endif // ATOOLS_IO_FILEROLLER_H
