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

#ifndef ATOOLS_SCENERY_FILERESOLVER_H
#define ATOOLS_SCENERY_FILERESOLVER_H

#include <QList>
#include <QStringList>
#include <QApplication>

namespace atools {
namespace fs {
class NavDatabaseOptions;
namespace scenery {

class SceneryArea;

/*
 * Collects all BGL files for a scenery area considering include and exclude configuration options.
 */
class FileResolver
{
  Q_DECLARE_TR_FUNCTIONS(FileResolver)

public:
  /*
   * @param opts configuration optios
   * @param noWarnings do not print warning messages if files could not be found
   */
  FileResolver(const atools::fs::NavDatabaseOptions& opts, bool noWarnings = false);
  virtual ~FileResolver();

  /*
   * Resolve and get all files for a scenery area
   * @param area scenery area to get the BGL files from
   * @param filepaths If not null will get all filepaths (path and filename)
   * @param filenames If not null will get all filenames (only filename)
   * @return number of files found
   */
  int getFiles(const atools::fs::scenery::SceneryArea& area, QStringList *filepaths = nullptr,
               QStringList *filenames = nullptr);

  const QStringList& getErrorMessages() const
  {
    return errorMessages;
  }

private:
  QStringList errorMessages;
  const atools::fs::NavDatabaseOptions& options;
  bool quiet = false;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_SCENERY_FILERESOLVER_H
