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

#ifndef ATOOLS_GEO_CALCULATIONS_H
#define ATOOLS_GEO_CALCULATIONS_H

#include <cmath>

namespace atools {
namespace geo {

/* Round to precision (e.g. roundToPrecision(1111, 2) -> 1100) */
template<typename TYPE>
int roundToPrecision(TYPE value, int precision = 0)
{
  if(precision == 0)
    return static_cast<int>(round(value));
  else
  {
    int factor = static_cast<int>(pow(10., precision));
    return static_cast<int>(round(value / factor)) * factor;
  }
}

/* Distance from nautical miles to meters */
template<typename TYPE>
TYPE nmToMeter(TYPE nm)
{
  return static_cast<TYPE>(static_cast<double>(nm) * 1852.216);
}

/* Distance from meters to nautical miles */
template<typename TYPE>
TYPE meterToNm(TYPE nm)
{
  return static_cast<TYPE>(static_cast<double>(nm) / 1852.216);
}

template<typename TYPE>
TYPE meterToFeet(TYPE value)
{
  return static_cast<TYPE>(3.2808399 * static_cast<double>(value));
}

template<typename TYPE>
TYPE feetToMeter(TYPE value)
{
  return static_cast<TYPE>(0.3048 * static_cast<double>(value));
}

template<typename TYPE>
TYPE feetToNm(TYPE value)
{
  return meterToNm(feetToMeter(value));
}

template<typename TYPE>
TYPE nmToRad(TYPE value)
{
  return static_cast<TYPE>(M_PI / (180. * 60.) * static_cast<double>(value));
}

template<typename TYPE>
TYPE toRadians(TYPE deg)
{
  return static_cast<TYPE>(static_cast<double>(deg) * 0.017453292519943295769236907684886);
}

template<typename TYPE>
TYPE toDegree(TYPE rad)
{
  return static_cast<TYPE>(static_cast<double>(rad) / 0.017453292519943295769236907684886);
}

template<typename TYPE>
TYPE opposedCourseDeg(TYPE rad)
{
  double result = static_cast<double>(rad) + 180.;
  while(result > 360.)
    result -= 360.;
  while(result < 360.)
    result += 360.;

  return static_cast<TYPE>(result);
}

template<typename TYPE>
TYPE normalizeLonXDeg(TYPE lonX)
{
  double result = static_cast<double>(lonX);
  while(result > 180.)
    result = result - 360.;
  while(result < -180.)
    result = result + 360.;
  return static_cast<TYPE>(result);
}

template<typename TYPE>
TYPE normalizeLatYDeg(TYPE latY)
{
  double result = static_cast<double>(latY);
  while(result > 90.)
    result = result - 180.;
  while(result < -90.)
    result = result + 180.;
  return static_cast<TYPE>(result);
}

} /* namespace geo */
} // namespace atools

#endif /* ATOOLS_GEO_CALCULATIONS_H */
