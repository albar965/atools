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

#ifndef INI_INIREADER_H_
#define INI_INIREADER_H_

#include <QString>
#include <QApplication>

namespace atools {
namespace fs {
namespace scenery {

class IniReader
{
  Q_DECLARE_TR_FUNCTIONS(atools::fs::scenery::IniReader)

public:
  IniReader()
    : currentLineNum(0)
  {
  }

  virtual ~IniReader();

  void read(const QString& iniFilename);

protected:
  virtual void onStartDocument(const QString& filename) = 0;
  virtual void onEndDocument(const QString& filename) = 0;
  virtual void onStartSection(const QString& name, const QString& nameSuffix) = 0;
  virtual void onEndSection(const QString& section, const QString& sectionSuffix) = 0;
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value) = 0;

  void throwException(const QString& message);

private:
  int currentLineNum;
  QString currentLine, currentSection, currentSectionSuffix, filename;

  void handleComment();
  void handleKeyValue();
  void handleSection();

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* INI_INIREADER_H_ */
