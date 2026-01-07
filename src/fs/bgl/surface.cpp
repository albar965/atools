/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/surface.h"

#include <QString>
#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

namespace surface {

bool isHard(atools::fs::bgl::Surface surface)
{
  if(surface == CONCRETE)
    return true;
  else if(surface == CEMENT)
    return true;
  else if(surface == ASPHALT)
    return true;
  else if(surface == BITUMINOUS)
    return true;
  else if(surface == TARMAC)
    return true;

  return false;
}

bool isWater(atools::fs::bgl::Surface surface)
{
  return surface == WATER;
}

bool isSoft(atools::fs::bgl::Surface surface)
{
  return !isWater(surface) && !isHard(surface);
}

QString surfaceToDbStr(const QString& surface)
{
  if(surface == "CONCRETE")
    return "C";
  else if(surface == "GRASS" || surface == "ERASE_GRASS")
    return "G";
  else if(surface == "WATER")
    return "W";
  else if(surface == "ASPHALT")
    return "A";
  else if(surface == "CEMENT")
    return "CE";
  else if(surface == "CLAY")
    return "CL";
  else if(surface == "SNOW")
    return "SN";
  else if(surface == "ICE")
    return "I";
  else if(surface == "DIRT")
    return "D";
  else if(surface == "CORAL")
    return "CR";
  else if(surface == "GRAVEL")
    return "GR";
  else if(surface == "OIL_TREATED" || surface == "PAINT")
    return "OT";
  else if(surface == "STEEL_MATS")
    return "SM";
  else if(surface == "BITUMINOUS")
    return "B";
  else if(surface == "BRICK")
    return "BR";
  else if(surface == "MACADAM")
    return "M";
  else if(surface == "PLANKS")
    return "PL";
  else if(surface == "SAND")
    return "S";
  else if(surface == "SHALE")
    return "SH";
  else if(surface == "TARMAC")
    return "T";
  else if(surface == "TRANSPARENT")
    return "TR";
  else if(surface == "UNDEFINED")
    return "UNKNOWN";

  // else if(surface == "UNKNOWN")
  // return "UNKNOWN";

  return "UNKNOWN";
}

atools::fs::bgl::Surface  surfaceToType(const QString& surface)
{
  if(surface == "CONCRETE")
    return CONCRETE;
  else if(surface == "GRASS" || surface == "ERASE_GRASS")
    return GRASS;
  else if(surface == "WATER")
    return WATER;
  else if(surface == "ASPHALT")
    return ASPHALT;
  else if(surface == "CEMENT")
    return CEMENT;
  else if(surface == "CLAY")
    return CLAY;
  else if(surface == "SNOW")
    return SNOW;
  else if(surface == "ICE")
    return ICE;
  else if(surface == "DIRT")
    return DIRT;
  else if(surface == "CORAL")
    return CORAL;
  else if(surface == "GRAVEL")
    return GRAVEL;
  else if(surface == "OIL_TREATED" || surface == "PAINT")
    return OIL_TREATED;
  else if(surface == "STEEL_MATS")
    return STEEL_MATS;
  else if(surface == "BITUMINOUS")
    return BITUMINOUS;
  else if(surface == "BRICK")
    return BRICK;
  else if(surface == "MACADAM")
    return MACADAM;
  else if(surface == "PLANKS")
    return PLANKS;
  else if(surface == "SAND")
    return SAND;
  else if(surface == "SHALE")
    return SHALE;
  else if(surface == "TARMAC")
    return TARMAC;
  else if(surface == "TRANSPARENT")
    return UNKNOWN;
  else if(surface == "UNDEFINED")
    return UNKNOWN;
  else if(surface == "UNKNOWN")
    return UNKNOWN;

  return UNKNOWN;
}

QString surfaceToDbStr(Surface surface)
{
  switch(surface)
  {
    case CONCRETE:
      return "C";

    case GRASS:
      return "G";

    case WATER:
      return "W";

    case ASPHALT:
      return "A";

    case CEMENT:
      return "CE";

    case CLAY:
      return "CL";

    case SNOW:
      return "SN";

    case ICE:
      return "I";

    case DIRT:
      return "D";

    case CORAL:
      return "CR";

    case GRAVEL:
      return "GR";

    case OIL_TREATED:
      return "OT";

    case STEEL_MATS:
      return "SM";

    case BITUMINOUS:
      return "B";

    case BRICK:
      return "BR";

    case MACADAM:
      return "M";

    case PLANKS:
      return "PL";

    case SAND:
      return "S";

    case SHALE:
      return "SH";

    case TARMAC:
      return "T";

    // X-Plane
    // case TRANSPARENT:
    // return "TR";

    case UNKNOWN:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

} // namespace surface
} // namespace bgl
} // namespace fs
} // namespace atools
