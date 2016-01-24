/*
 * SceneryCfg.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef INI_INIREADER_H_
#define INI_INIREADER_H_

#include <QString>

namespace atools {
namespace fs {
namespace scenery {

class IniReader
{
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
  virtual void onKeyValue(const QString& section,
                          const QString& sectionSuffix,
                          const QString& key,
                          const QString& value) = 0;

  [[noreturn]] void throwException(const QString& message);

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
