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

namespace atools {
namespace geo {

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
TYPE toRadians(TYPE deg)
{
  return static_cast<TYPE>(static_cast<double>(deg) * 0.017453292519943295769236907684886);
}

template<typename TYPE>
TYPE toDegree(TYPE rad)
{
  return static_cast<TYPE>(static_cast<double>(rad) / 0.017453292519943295769236907684886);
}

} /* namespace geo */
} // namespace atools

#endif /* ATOOLS_GEO_CALCULATIONS_H */
