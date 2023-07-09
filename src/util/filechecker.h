/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FILECHECKER_H
#define ATOOLS_FILECHECKER_H

#include <QString>

namespace atools {
namespace util {

/*
 * Implements delayed file checking to avoid file accesses.
 * Uses atools::checkFile()
 */
class FileChecker
{
public:
  /* Acesses filesystem only if filename differs */
  bool checkFile(const QString& funcInfo, const QString& file, bool warn);

  /* Acesses filesystem only if dirname differs */
  bool checkDir(const QString& funcInfo, const QString& dir, bool warn);

  /* Last checked file or dirname. Empty if nothing was checked yet. */
  const QString& getPath() const
  {
    return path;
  }

  /* Last result. False if nothing was checked yet. */
  bool isValid() const
  {
    return valid;
  }

private:
  QString path;
  bool valid = false;

};

} // namespace util
} // namespace atools

#endif // ATOOLS_FILECHECKER_H
