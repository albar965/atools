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

#include "fs/perf/aircraftperfconstants.h"

namespace atools {
namespace fs {
namespace perf {

QString runwayTypeToStr(RunwayType type)
{
  switch(type)
  {
    case atools::fs::perf::SOFT:
      return "SOFT";

    case atools::fs::perf::HARD:
      return "HARD";

    case atools::fs::perf::WATER:
      return "WATER";

    case atools::fs::perf::WATER_LAND:
      return "WATERLAND";
  }
  return QString();
}

RunwayType runwayTypeFromStr(QString str)
{
  str = str.toLower();

  if(str == "soft")
    return atools::fs::perf::SOFT;
  else if(str == "hard")
    return atools::fs::perf::HARD;
  else if(str == "water")
    return atools::fs::perf::WATER;
  else if(str == "waterland")
    return atools::fs::perf::WATER_LAND;
  else
    return atools::fs::perf::SOFT;
}

} // namespace perf
} // namespace fs
} // namespace atools
