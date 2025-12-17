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

#ifndef ATOOLS_UTIL_CSVFILEREADER_H
#define ATOOLS_UTIL_CSVFILEREADER_H

#include <QStringList>
#include <QList>

class QTextStream;

namespace atools {
namespace util {

class CsvReader;

/*
 * Provides functionality to read a full CSV file including escaped lines with linefeeds and more into a string list vector.
 */
class CsvFileReader
{
public:
  CsvFileReader();
  /* trimValues: Trims only text which is not escaped */
  CsvFileReader(QChar separatorChar, QChar escapeChar, bool trimValues);
  ~CsvFileReader();

  /* Read a CSV stream considering escape characters and double escape characters.
   * Content can be fetched after reading by getValues() */
  void readCsvFile(QTextStream& stream);
  void readCsvFile(const QString& filename);

  /* Get values after calling readCsvFile */
  const QList<QStringList>& getValues() const
  {
    return values;
  }

  /* Clears values */
  void reset()
  {
    values.clear();
  }

private:
  /* List of row and column values */
  QList<QStringList> values;
  atools::util::CsvReader *reader = nullptr;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_CSVFILEREADER_H
