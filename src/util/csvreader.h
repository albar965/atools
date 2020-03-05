/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_UTIL_CSVREADER_H
#define ATOOLS_UTIL_CSVREADER_H

#include <QStringList>

namespace atools {
namespace util {

/*
 * Provides functionality to read CSV files including escaped lines with linefeeds and more.
 */
class CsvReader
{
public:
  CsvReader();
  /* trimValues: Trims only text which is not escaped */
  CsvReader(QChar separatorChar, QChar escapeChar, bool trimValues);
  ~CsvReader();

  /* Read a CSV line considering escape characters and double escape characters. This allows linefeeds in fields.
   * Example: value1,"value2 with , separator",value3,"value4 with "" escaped",value4*/
  void readCsvLine(const QString& line);

  /* Get values after calling readCsvLine once or more */
  const QStringList& getValues() const
  {
    return values;
  }

  /* inEscape is set if the line ended but is still escaped - continue reading lines until false */
  bool isInEscape() const
  {
    return inEscape;
  }

  /* Reset all internal states to allow reuse */
  void reset();

private:
  /* State */

  /* List of column values */
  QStringList values;

  /* Current column value read */
  QString curValue;

  bool inEscape = false, /* Currently in escaped text */
       curValueEscaped = false; /* Current value read is in escape - do no trim */

  QChar lastChar = '\0', curChar = '\0';

  /* Configuration */
  QChar separator = ',', escape = '"';
  bool trim = true;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_CSVREADER_H
