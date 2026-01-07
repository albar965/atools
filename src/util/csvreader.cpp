/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "util/csvreader.h"

namespace atools {
namespace util {

CsvReader::CsvReader()
{

}

CsvReader::CsvReader(QChar separatorChar, QChar escapeChar, bool trimValues)
  : separator(separatorChar), escape(escapeChar), trim(trimValues)
{

}

CsvReader::~CsvReader()
{

}

void CsvReader::readCsvLine(const QString& line)
{
  if(!inEscape)
    // Reading a full new line
    reset();
  else
    // In escape - add e new line
    curValue += "\n";

  for(int i = 0; i < line.size(); i++)
  {
    curChar = line.at(i);

    if(curChar == escape)
    {
      // Remember if this value is escaped to suppress trimming
      curValueEscaped = true;

      // Found escape character "
      if(inEscape)
        // End of escaped text
        inEscape = false;
      else
      {
        if(lastChar == escape)
          // Escape char itself doubled "" - add single escape " to value and keep escaped state
          curValue.append(curChar);
        inEscape = true;
      }
      lastChar = curChar;

      // Do not store value
      continue;
    }

    if(curChar == separator && !inEscape)
    {
      // Separator in unescaped text - start new value
      values.append((trim && !curValueEscaped) ? curValue.trimmed() : curValue);
      curValue.clear();
      curValueEscaped = false;
      lastChar = curChar;
      continue;
    }

    // Regular character
    curValue.append(curChar);
    lastChar = curChar;
  }

  if(!inEscape)
  {
    // Finishe line
    values.append((trim && !curValueEscaped) ? curValue.trimmed() : curValue);
    curValue.clear();
    curValueEscaped = false;
  }
}

void CsvReader::reset()
{
  values.clear();
  inEscape = false;
  curValue.clear();
  lastChar = '\0';
  curChar = '\0';
  curValueEscaped = false;
}

} // namespace util
} // namespace atools
