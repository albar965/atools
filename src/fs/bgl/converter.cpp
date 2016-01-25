/*
 * Converter.cpp
 *
 *  Created on: 20.04.2015
 *      Author: alex
 */

#include "fs/bgl/converter.h"
#include "fs/bgl/bglexception.h"

namespace atools {
namespace fs {
namespace bgl {
namespace converter {

QString intToIcao(unsigned int icao, bool noBitShift)
{
  QString icaoStr;
  unsigned int value = icao;
  // The ICAO identifiers for primary and secondary ILS in a runway record are not shifted.
  if(!noBitShift)
    value = value >> 5;

  // if(icao > 0 && value == 0)
  // qWarning().nospace().noquote() << "Icao is null after shift. Was " << icao;

  if(value == 0)
    return "";

  unsigned int codedArr[5] = {0, 0, 0, 0, 0};
  unsigned int coded = 0;

  int idx = 0;
  if(value > 37)
    while(value > 37)
    {
      coded = value % 38;
      codedArr[idx++] = coded;
      value = (value - coded) / 38;
      if(value < 38)
      {
        coded = value;
        codedArr[idx++] = coded;
      }
    }
  else
    codedArr[idx++] = value;

  for(int i = 0; i < 5; i++)
  {
    coded = codedArr[i];
    if(coded == 0)
      break;
    if(coded > 1 && coded < 12)
      icaoStr.insert(0, '0' + (coded - 2));
    else
      icaoStr.insert(0, 'A' + (coded - 12));
  }
  return icaoStr;
}

QString designatorStr(int designator)
{
  static const char *designators[] = {"", "L", "R", "C", "W", "A", "B"};

  if(designator >= 0 && designator <= 6)
    return designators[designator];
  else
    throw BglException(
            "Value for designator out of range in designatorStr(): " + QString::number(designator));
}

QString runwayToStr(int runwayNumber, int designator)
{
  QString retval;
  if(runwayNumber < 10)
  {
    retval += "0";
    retval += static_cast<char>(runwayNumber) + '0';
  }
  else if(runwayNumber > 36)
  {
    switch(runwayNumber)
    {
      case 37:
        return "N";

      case 38:
        return "NE";

      case 39:
        return "E";

      case 40:
        return "SE";

      case 41:
        return "S";

      case 42:
        return "SW";

      case 43:
        return "W";

      case 44:
        return "NW";

      default:
        throw BglException("Runway number out of range in runwayToStr(): " +
                           QString::number(runwayNumber));
    }
  }
  else
  {
    retval += static_cast<char>(runwayNumber / 10) + '0';
    retval += static_cast<char>(runwayNumber % 10) + '0';
  }
  retval += designatorStr(designator);
  return retval;

}

time_t filetime(unsigned int lowDateTime, unsigned int highDateTime)
{
  // number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
  static const unsigned long long FILETIME_EPOCH_DIFF = 11644473600LL;
  static const unsigned long long FILETIME_SECOND = 10000000LL;

  unsigned long long filetime = ((static_cast<unsigned long long>(highDateTime)) << 32)
                                + static_cast<unsigned long long>(lowDateTime);
  filetime /= FILETIME_SECOND;
  filetime -= FILETIME_EPOCH_DIFF;
  return static_cast<time_t>(filetime);
}

} // namespace  converter
} // namespace bgl
} // namespace fs
} // namespace atools
