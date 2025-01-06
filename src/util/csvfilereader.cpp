/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "util/csvfilereader.h"

#include "atools.h"
#include "util/csvreader.h"

namespace atools {
namespace util {

CsvFileReader::CsvFileReader()
{
  reader = new CsvReader();
}

CsvFileReader::CsvFileReader(QChar separatorChar, QChar escapeChar, bool trimValues)
{
  reader = new CsvReader(separatorChar, escapeChar, trimValues);
}

CsvFileReader::~CsvFileReader()
{
  ATOOLS_DELETE(reader);
}

void CsvFileReader::readCsvFile(QTextStream& stream)
{
  while(!stream.atEnd())
  {
    QString line = stream.readLine();

    // Skip empty lines but add them if within an escaped field
    if(line.isEmpty() && !reader->isInEscape())
      continue;

    reader->readCsvLine(line);
    if(reader->isInEscape())
      // Still in an escaped line so continue to read unchanged until " shows the end of the field
      continue;

    values.append(reader->getValues());
  }
}

} // namespace util
} // namespace atools
