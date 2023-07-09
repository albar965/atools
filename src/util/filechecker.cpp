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

#include "util/filechecker.h"

#include "atools.h"

namespace atools {
namespace util {

bool FileChecker::checkFile(const QString& funcInfo, const QString& file, bool warn)
{
  if(path != file)
  {
    path = file;
    valid = atools::checkFile(funcInfo, path, warn);
  }

  return valid;
}

bool FileChecker::checkDir(const QString& funcInfo, const QString& dir, bool warn)
{
  if(dir != path)
  {
    path = dir;
    valid = atools::checkDir(funcInfo, path, warn);
  }

  return valid;
}

} // namespace util
} // namespace atools
