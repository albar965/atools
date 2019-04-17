/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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
 * Line comments starting with ";" are supported per default.
 */
class AbstractIniReader
{
  Q_DECLARE_TR_FUNCTIONS(atools::fs::scenery::IniReader)

public:
  AbstractIniReader(const QString& textCodec = QString());
  virtual ~AbstractIniReader();

  /* Read the file and trigger the on* methods */
  void read(const QString& iniFilename);

  /* Set characters that are used to start a comment. Default is ";" */
  void setCommentCharacters(const QStringList& value)
  {
    commentCharacters = value;
  }

  const QStringList& getCommentCharacters() const
  {
    return commentCharacters;
  }

  /* All section and key names are converted to lower case if false which is default. */
  void setPreserveCase(bool value)
  {
    preserveCase = value;
  }

protected:
  /*
   * Called on reading the document
   * @param filename
   */
  virtual void onStartDocument(const QString& filepath);

  /*
   * Called at document end
   * @param filename
   */
  virtual void onEndDocument(const QString& filepath);

  /*
   * Called when a section in square brackets starts
   * @param name section name in square brackets
   * @param nameSuffix part of the section name after a dot
   */
  virtual void onStartSection(const QString& name, const QString& nameSuffix);

  /*
   * Called when a section in square brackets ends
   * @param name section name in square brackets
   * @param nameSuffix part of the section name after a dot
   */
  virtual void onEndSection(const QString& section, const QString& sectionSuffix);

  /*
   * Called when a key = value pair was found. Both methods are called. Default implementation does nothing.
   * @param section section name in square brackets
   * @param sectionSuffix part of the section name after a dot
   * @param key before equal sign
   * @param value after equal sign
   */
  virtual void onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value);
  virtual void onKeyValue(const QString& section, const QString& key, const QString& value);

  /* Throw an exception with file and line information */
#if defined(Q_CC_MSVC)
  void throwException(const QString& message);

#else
  [[noreturn]]  void throwException(const QString& message);

#endif

protected:
  QString filepath;

  bool toBool(const QString& str);
  int toInt(const QString& str);

private:
  int currentLineNum;
  QString currentLine, currentSection, currentSectionSuffix;

  void handleComment();
  void handleKeyValue();
  void handleSection();
  QString changeCase(const QString& str);

  QString codec;
  QStringList commentCharacters;
  bool preserveCase = false;
};

} // namespace io
} // namespace atools

#endif // ATOOLS_INI_INIREADER_H
