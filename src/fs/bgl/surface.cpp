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
  if(surface == QStringLiteral("CONCRETE"))
    return QStringLiteral("C");
  else if(surface == QStringLiteral("GRASS") || surface == QStringLiteral("ERASE_GRASS"))
    return QStringLiteral("G");
  else if(surface == QStringLiteral("WATER"))
    return QStringLiteral("W");
  else if(surface == QStringLiteral("ASPHALT"))
    return QStringLiteral("A");
  else if(surface == QStringLiteral("CEMENT"))
    return QStringLiteral("CE");
  else if(surface == QStringLiteral("CLAY"))
    return QStringLiteral("CL");
  else if(surface == QStringLiteral("SNOW"))
    return QStringLiteral("SN");
  else if(surface == QStringLiteral("ICE"))
    return QStringLiteral("I");
  else if(surface == QStringLiteral("DIRT"))
    return QStringLiteral("D");
  else if(surface == QStringLiteral("CORAL"))
    return QStringLiteral("CR");
  else if(surface == QStringLiteral("GRAVEL"))
    return QStringLiteral("GR");
  else if(surface == QStringLiteral("OIL_TREATED") || surface == QStringLiteral("PAINT"))
    return QStringLiteral("OT");
  else if(surface == QStringLiteral("STEEL_MATS"))
    return QStringLiteral("SM");
  else if(surface == QStringLiteral("BITUMINOUS"))
    return QStringLiteral("B");
  else if(surface == QStringLiteral("BRICK"))
    return QStringLiteral("BR");
  else if(surface == QStringLiteral("MACADAM"))
    return QStringLiteral("M");
  else if(surface == QStringLiteral("PLANKS"))
    return QStringLiteral("PL");
  else if(surface == QStringLiteral("SAND"))
    return QStringLiteral("S");
  else if(surface == QStringLiteral("SHALE"))
    return QStringLiteral("SH");
  else if(surface == QStringLiteral("TARMAC"))
    return QStringLiteral("T");
  else if(surface == QStringLiteral("TRANSPARENT"))
    return QStringLiteral("TR");
  else if(surface == QStringLiteral("UNDEFINED"))
    return QStringLiteral("UNKNOWN");

  // else if(surface == "UNKNOWN")
  // return "UNKNOWN";

  return QStringLiteral("UNKNOWN");
}

atools::fs::bgl::Surface  surfaceToType(const QString& surface)
{
  if(surface == QStringLiteral("CONCRETE"))
    return CONCRETE;
  else if(surface == QStringLiteral("GRASS") || surface == QStringLiteral("ERASE_GRASS"))
    return GRASS;
  else if(surface == QStringLiteral("WATER"))
    return WATER;
  else if(surface == QStringLiteral("ASPHALT"))
    return ASPHALT;
  else if(surface == QStringLiteral("CEMENT"))
    return CEMENT;
  else if(surface == QStringLiteral("CLAY"))
    return CLAY;
  else if(surface == QStringLiteral("SNOW"))
    return SNOW;
  else if(surface == QStringLiteral("ICE"))
    return ICE;
  else if(surface == QStringLiteral("DIRT"))
    return DIRT;
  else if(surface == QStringLiteral("CORAL"))
    return CORAL;
  else if(surface == QStringLiteral("GRAVEL"))
    return GRAVEL;
  else if(surface == QStringLiteral("OIL_TREATED") || surface == QStringLiteral("PAINT"))
    return OIL_TREATED;
  else if(surface == QStringLiteral("STEEL_MATS"))
    return STEEL_MATS;
  else if(surface == QStringLiteral("BITUMINOUS"))
    return BITUMINOUS;
  else if(surface == QStringLiteral("BRICK"))
    return BRICK;
  else if(surface == QStringLiteral("MACADAM"))
    return MACADAM;
  else if(surface == QStringLiteral("PLANKS"))
    return PLANKS;
  else if(surface == QStringLiteral("SAND"))
    return SAND;
  else if(surface == QStringLiteral("SHALE"))
    return SHALE;
  else if(surface == QStringLiteral("TARMAC"))
    return TARMAC;
  else if(surface == QStringLiteral("TRANSPARENT"))
    return UNKNOWN;
  else if(surface == QStringLiteral("UNDEFINED"))
    return UNKNOWN;
  else if(surface == QStringLiteral("UNKNOWN"))
    return UNKNOWN;

  return UNKNOWN;
}

QString surfaceToDbStr(Surface surface)
{
  switch(surface)
  {
    case CONCRETE:
      return QStringLiteral("C");

    case GRASS:
      return QStringLiteral("G");

    case WATER:
      return QStringLiteral("W");

    case ASPHALT:
      return QStringLiteral("A");

    case CEMENT:
      return QStringLiteral("CE");

    case CLAY:
      return QStringLiteral("CL");

    case SNOW:
      return QStringLiteral("SN");

    case ICE:
      return QStringLiteral("I");

    case DIRT:
      return QStringLiteral("D");

    case CORAL:
      return QStringLiteral("CR");

    case GRAVEL:
      return QStringLiteral("GR");

    case OIL_TREATED:
      return QStringLiteral("OT");

    case STEEL_MATS:
      return QStringLiteral("SM");

    case BITUMINOUS:
      return QStringLiteral("B");

    case BRICK:
      return QStringLiteral("BR");

    case MACADAM:
      return QStringLiteral("M");

    case PLANKS:
      return QStringLiteral("PL");

    case SAND:
      return QStringLiteral("S");

    case SHALE:
      return QStringLiteral("SH");

    case TARMAC:
      return QStringLiteral("T");

    // X-Plane
    // case TRANSPARENT:
    // return "TR";

    case UNKNOWN:
      return QStringLiteral("UNKNOWN");
  }
  return QStringLiteral("UNKNOWN");
}

} // namespace surface
} // namespace bgl
} // namespace fs
} // namespace atools
