/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_INI_INIREADER_H
#define ATOOLS_INI_INIREADER_H

#include <QString>
#include <QCoreApplication>

namespace atools {
namespace io {

/*
 * Abstract class that can read ini files and supports numbered sections like [area.001].
 * Line comments starting with "#" and ";" are supported.
 */
class IniReader
{
  Q_DECLARE_TR_FUNCTIONS(atools::fs::scenery::IniReader)

public:
  IniReader(const QString& textCodec = QString());
  virtual ~IniReader();

  /* Read the file and trigger the on* methods */
  void read(const QString& iniFilename);

protected:
  /*
   * Called on reading the document
   * @param filename
   */
  virtual void onStartDocument(const QString& filename) = 0;

  /*
   * Called at document end
   * @param filename
   */
  virtual void onEndDocument(const QString& filename) = 0;

  /*
   * Called when a section in square brackets starts
   * @param name section name in square brackets
   * @param nameSuffix part of the section name after a dot
   */
  virtual void onStartSection(const QString& name, const QString& nameSuffix) = 0;

  /*
   * Called when a section in square brackets ends
   * @param name section name in square brackets
   * @param nameSuffix part of the section name after a dot
   */
  virtual void onEndSection(const QString& section, const QString& sectionSuffix) = 0;

  /*
   * Called when a key = value pair was found
   * @param section section name in square brackets
   * @param sectionSuffix part of the section name after a dot
   * @param key before equal sign
   * @param value after equal sign
   */
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value) = 0;

  /* Throw an exception with file and line information */
#if defined(Q_CC_MSVC)
  void throwException(const QString& message);

#else
  [[noreturn]]  void throwException(const QString& message);

#endif

protected:
  QString filename;

private:
  int currentLineNum;
  QString currentLine, currentSection, currentSectionSuffix;

  void handleComment();
  void handleKeyValue();
  void handleSection();

  QString codec;
};

} // namespace io
} // namespace atools

#endif // ATOOLS_INI_INIREADER_H
