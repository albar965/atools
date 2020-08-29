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

#ifndef SURFACE_H
#define SURFACE_H

class QString;

namespace atools {
namespace fs {
namespace bgl {

/* Surface - also used for aprons and taxiways */
enum Surface
{
  CONCRETE = 0x0000,
  GRASS = 0x0001,
  WATER = 0x0002,
  CEMENT = 0x0003, // TODO wiki error report
  ASPHALT = 0x0004,
  // GRASS = 0x0005, invalid ADE interprets it as grass
  CLAY = 0x0007,
  SNOW = 0x0008,
  ICE = 0x0009,
  DIRT = 0x000C,
  CORAL = 0x000D,
  GRAVEL = 0x000E,
  OIL_TREATED = 0x000F,
  STEEL_MATS = 0x0010,
  BITUMINOUS = 0x0011,
  BRICK = 0x0012,
  MACADAM = 0x0013,
  PLANKS = 0x0014,
  SAND = 0x0015,
  SHALE = 0x0016,
  TARMAC = 0x0017,
  UNKNOWN = 0x00FE
};

/* Remove upper bit from surface */
const int SURFACE_MASK = 0x7F;

namespace surface {
bool isWater(atools::fs::bgl::Surface value);
bool isSoft(atools::fs::bgl::Surface value);
bool isHard(atools::fs::bgl::Surface value);

/* Type to short mostly two letter strings for the database */
QString surfaceToDbStr(atools::fs::bgl::Surface value);
QString surfaceToDbStr(const QString& surface);

/* Long string from MSFS material definitions to short database string */
atools::fs::bgl::Surface surfaceToType(const QString& surface);

}

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // SURFACE_H
