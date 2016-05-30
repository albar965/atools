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
#include <limits>

#include <QString>

namespace atools {
namespace geo {

enum LineDist
{
  DIST_TO_LINE,
  DIST_TO_START,
  DIST_TO_END
};

float distanceToLine(float x, float y, float x1, float y1, float x2, float y2, bool lineOnly = false,
                     atools::geo::LineDist *distType = nullptr);

template<typename TYPE>
int manhattanDistance(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

template<typename TYPE>
int simpleDistance(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return static_cast<int>(std::round(sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)))));
}

template<typename TYPE>
float simpleDistanceF(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return static_cast<float>(sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
}

/* Temperature from celsius to farenheit */
template<typename TYPE>
TYPE degCToDegF(TYPE temp)
{
  return static_cast<TYPE>(1.8 * static_cast<double>(temp) + 32);
}

/* Temperature from farenheit to celsius */
template<typename TYPE>
TYPE degFToDegC(TYPE temp)
{
  return static_cast<TYPE>((static_cast<double>(temp) - 32) / 1.8);
}

/* Pressure from millibar to inches Hg */
template<typename TYPE>
TYPE mbarToInHg(TYPE press)
{
  return static_cast<TYPE>(static_cast<double>(press) * 0.02953);
}

/* Pressure from inches Hg to millibar */
template<typename TYPE>
TYPE inHgToMbar(TYPE press)
{
  return static_cast<TYPE>(static_cast<double>(press) * 33.863753);
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
TYPE meterToRad(TYPE value)
{
  return nmToRad(meterToNm(value));
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
TYPE normalizeCourse(TYPE degree)
{
  double result = static_cast<double>(degree);
  while(result > 360.)
    result = result - 360.;
  while(result < 0.)
    result = result + 360.;
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
