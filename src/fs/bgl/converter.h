/*
 * Converter.h
 *
 *  Created on: 20.04.2015
 *      Author: alex
 */

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
