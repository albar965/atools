/*
 * SceneryCfg.cpp
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#include "fs/scenery/inireader.h"
#include "exception.h"

#include <QTextStream>
#include <QIODevice>
#include <QFile>

namespace atools {
namespace fs {
namespace scenery {

void IniReader::throwException(const QString& message)
{
  throw Exception(message + ". File \"" + filename + "\", line " + QString::number(currentLineNum) +
                  ":\"" + currentLine + "\"");
}

IniReader::~IniReader()
{
}

void IniReader::handleComment()
{
  int c = currentLine.indexOf('#');
  if(c >= 0)
    currentLine.truncate(c);

  c = currentLine.indexOf(';');
  if(c >= 0)
    currentLine.truncate(c);
}

void IniReader::handleKeyValue()
{
  QString name, value;

  int c = currentLine.indexOf('=');
  if(c >= 0)
  {
    name = currentLine.left(c).toLower();
    if(name.isEmpty())
      throwException("Missing key name before \"=\"");
    value = currentLine.mid(c + 1);
  }
  else
    throwException("Missing \"=\"");

  onKeyValue(currentSection, currentSectionSuffix, name, value);
}

void IniReader::handleSection()
{
  QString tempSection, tempSectionSuffix;

  if(currentLine.at(currentLine.size() - 1) == ']')
    tempSection = currentLine.mid(1, currentLine.size() - 2);
  else
    throwException("Missing closing \"]\"");

  int dotPos = tempSection.indexOf('.');
  if(dotPos >= 0)
  {
    QString sectPrefix = tempSection.mid(0, dotPos).toLower();
    if(sectPrefix.isEmpty())
      throwException("Missing section name before \".\"");

    tempSectionSuffix = tempSection.mid(dotPos + 1).toLower();
    if(tempSectionSuffix.isEmpty())
      throwException("Missing section suffix after \".\"");

    tempSection = sectPrefix;
  }

  if(!currentSection.isEmpty())
    onEndSection(currentSection, currentSectionSuffix);

  currentSection = tempSection;
  currentSectionSuffix = tempSectionSuffix;
  onStartSection(currentSection, currentSectionSuffix);
}

void IniReader::read(const QString& iniFilename)
{
  currentSection.clear();
  currentSectionSuffix.clear();
  currentLine.clear();
  currentLineNum = 0;

  this->filename = iniFilename;

  QFile sceneryCfgFile(filename);

  if(sceneryCfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream sceneryCfg(&sceneryCfgFile);
    onStartDocument(filename);

    while(sceneryCfg.status() == QTextStream::Ok)
    {
      currentLine.clear();

      sceneryCfg.readLineInto(&currentLine, 1024);
      currentLine = currentLine.trimmed();
      currentLineNum++;

      handleComment();

      if(currentLine.isEmpty())
        continue;

      if(currentLine.at(0) == '[')
        handleSection();
      else
        handleKeyValue();
    }
    onEndDocument(filename);
    sceneryCfgFile.close();
  }
}

} // namespace scenery
} // namespace fs
} // namespace atools
