/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/navdatabaseerrors.h"

namespace atools {
namespace fs {

NavDatabaseErrors::NavDatabaseErrors()
{

}

int NavDatabaseErrors::getTotalErrors() const
{
  int total = 0;
  for(const atools::fs::NavDatabaseErrors::SceneryErrors& scErr :  sceneryErrors)
    total += scErr.fileErrors.size() + scErr.sceneryErrorsMessages.size();
  return total;
}

void NavDatabaseErrors::init(const atools::fs::scenery::SceneryArea& area)
{
  NavDatabaseErrors::SceneryErrors err;
  err.scenery = area;
  sceneryErrors.append(err);
}

} // namespace fs
} // namespace atools
