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

#include "fs/bgl/converter.h"
#include "exception.h"

#include <QVarLengthArray>
#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {
namespace converter {

static const char *RUNWAY_DESIGNATORS[] = {"", "L", "R", "C", "W", "A", "B"};

QString intToIcao(unsigned int icao, bool noBitShift)
{
  QString icaoStr;
  unsigned int value = icao;
  // The ICAO identifiers for primary and secondary ILS in a runway record are not shifted.
  if(!noBitShift)
    value = value >> 5;

  if(value == 0)
    return QString();

  QVarLengthArray<unsigned int, 5> codedArr({0, 0, 0, 0, 0});
  unsigned int coded = 0;

  // First extract the coded/compressed values
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

  // Convert the decompressed bytes to characters
  for(int i = 0; i < 5; i++)
  {
    coded = codedArr.at(i);
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

  if(designator >= 0 && designator <= 6)
    return RUNWAY_DESIGNATORS[designator];
  else
  {
    qWarning() << "Value for designator out of range in designatorStr()" << designator;
    return QString();
  }
}

QString runwayToStr(int runwayNumber, int designator)
{
  QString retval;
  if(runwayNumber < 10)
  {
    // Normal one digit runway number with leading zero
    retval += "0";
    retval += static_cast<char>(runwayNumber) + '0';
  }
  else if(runwayNumber > 36)
  {
    // Special runway number code
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
        qWarning() << "Runway number out of range in runwayToStr()" << runwayNumber;
    }
  }
  else
  {
    // Normal tow digit runway number without leading zero
    retval += static_cast<char>(runwayNumber / 10) + '0';
    retval += static_cast<char>(runwayNumber % 10) + '0';
  }
  // Add designator if there is one
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
