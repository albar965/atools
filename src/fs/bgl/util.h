/*
 * Util.h
 *
 *  Created on: 05.05.2015
 *      Author: alex
 */

#ifndef BGL_UTIL_H_
#define BGL_UTIL_H_

#include <QString>

namespace atools {
namespace fs {
namespace bgl {
namespace util {

template<typename TYPE>
QString enumToStr(QString func(TYPE t), TYPE type)
{
  QString retval = func(type);
  if(retval == "NONE" || retval == "NO")
    return "";
  else
    return retval;
}

inline int meterToNm(float meters)
{
  return static_cast<int>(round(meters / 1852.0f));
}

inline int meterToFeet(float meters, int precision = 0)
{
  if(precision == 0)
    return static_cast<int>(round(meters * 3.281f));

  {
    int factor = static_cast<int>(pow(10., precision));
    return static_cast<int>(round(meters * 3.281f / factor)) * factor;
  }
}

template<typename TYPE>
bool isFlagSet(TYPE bitfield, TYPE flag)
{
  return (bitfield & flag) == flag;
}

template<typename TYPE>
bool isFlagNotSet(TYPE bitfield, TYPE flag)
{
  return (bitfield & flag) == 0;
}

} // namespace util
} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_UTIL_H_ */
