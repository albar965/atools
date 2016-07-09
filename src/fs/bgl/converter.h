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

#ifndef ATOOLS_BGL_CONVERTER_H
#define ATOOLS_BGL_CONVERTER_H

#include <QString>
#include <time.h>

namespace atools {
namespace fs {
namespace bgl {
namespace converter {

/*
 * Converts the BGL specific coordinate format to degrees
 * @return longitude degrees
 */
inline float intToLonX(int lonX)
{
  return (lonX * (360.0f / (3.f * 0x10000000))) - 180.0f;
}

/*
 * Converts the BGL specific coordinate format to degrees
 * @return latitude degrees
 */
inline float intToLatY(int latY)
{
  return 90.0f - latY * (180.0f / (2.f * 0x10000000));
}

/* Get the time in seconds since epoch from the BGL header specific format */
time_t filetime(unsigned int lowDateTime, unsigned int highDateTime);

/*
 * Convert the BGL ICAO format to string
 * @param noBitShift if true do not shift 5 bits to the right
 */
QString intToIcao(unsigned int icao, bool noBitShift = false);

/*
 * Convert BGL runway designator to a string like "L", "C", "R" or "W"
 */
QString designatorStr(int designator);

/*
 * Create a full runway name from number and designator.
 * @return Runway name like "12", "24C" or "NE"
 */
QString runwayToStr(int runwayNumber, int designator);

/*
 * Adjust FS magvar values to positive/negative values where value < 0 for West and value > 0 for East
 */
inline float adjustMagvar(float magVar)
{
  return -(magVar > 180.f ? magVar - 360.f : magVar);
}

} // namespace  converter
} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_CONVERTER_H
