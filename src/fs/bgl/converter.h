/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef BGL_CONVERTER_H_
#define BGL_CONVERTER_H_

#include <QString>
#include <time.h>

namespace atools {
namespace fs {
namespace bgl {
namespace converter {

inline float intToLonX(int lonX)
{
  return (lonX * (360.0f / (3.f * 0x10000000))) - 180.0f;
}

inline float intToLatY(int latY)
{
  return 90.0f - latY * (180.0f / (2.f * 0x10000000));
}

time_t filetime(unsigned int lowDateTime, unsigned int highDateTime);

QString intToIcao(unsigned int icao, bool noBitShift = false);

QString designatorStr(int designator);

QString runwayToStr(int runwayNumber, int designator);

} // namespace  converter
} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_CONVERTER_H_ */
