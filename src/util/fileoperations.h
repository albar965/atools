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

#ifndef ATOOLS_FILEOPERATIONS_H
#define ATOOLS_FILEOPERATIONS_H

#include <QCoreApplication>
#include <QStringList>

namespace atools {
namespace util {

/*
 * Provides operations to remove or copy folder structures recursively.
 * Collects error texts separately.
 */
class FileOperations
{
  Q_DECLARE_TR_FUNCTIONS(FileOperations)

public:
  /* Log all operations if verbose is true */
  FileOperations(bool verboseParam = false);

  /* Copies a folder structure recursively to the destination. Returns true if successfull.
   * Stops on first error. Overwrites present files if requested.
   * The parent of directory "to" has to exist. Always copying top dir "from" to "to".
   * Optionally copies hidden and system files.
   * Note that hidden and system might be required to copy symbolic links and broken links.*/
  void copyDirectory(const QString& from, const QString& to, bool overwrite = false, bool hidden = false, bool system = false);

  /* Deletes folders and files recursively. Returns true if successfull.
   * Continues in case of errors. Keeps folders if keepDirs is true.
   * Optionally removes hidden and system files.
   * Note that hidden and system might be required to remove symbolic links and broken links. */
  void removeDirectory(const QString& directory, bool keepDirs = false, bool hidden = false, bool system = false);

  /* Moves folder to system trash */
  void removeDirectoryToTrash(const QString& directory);

  /* Get errors from previous operation */
  const QStringList& getErrors() const
  {
    return errors;
  }

  /* Get number of files deleted or copied */
  int getFilesProcessed() const
  {
    return filesProcessed;
  }

  /* true if a folder is allowed to be removed. false for system folders like "Documents" and root folders. */
  bool canRemoveDir(const QString& dir) const;

  bool hasErrors() const
  {
    return !errors.isEmpty();
  }

private:
  void copyDirectoryInternal(const QString& from, const QString& to, bool overwrite, bool hidden, bool system);
  void removeDirectoryInternal(const QString& directory, bool keepDirs, bool hidden, bool system);

  QStringList errors, allDefaultPaths;
  bool verbose = false;
  int filesProcessed = 0;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_FILEOPERATIONS_H
