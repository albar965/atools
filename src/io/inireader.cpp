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

#include "io/inireader.h"
#include "exception.h"

#include <QDebug>
#include <QTextStream>
#include <QIODevice>
#include <QFile>

namespace atools {
namespace io {

IniReader::IniReader(const QString& textCodec)
  : currentLineNum(0), codec(textCodec)
{
}

IniReader::~IniReader()
{
}

void IniReader::handleComment()
{
  int c = currentLine.indexOf(';');
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
      qWarning() << "Missing key name before \"=\":" << currentLine;
    else
    {
      value = currentLine.mid(c + 1);
      onKeyValue(currentSection, currentSectionSuffix, name, value);
    }
  }
  else
    qWarning() << "Missing \"=\":" << currentLine;
}

void IniReader::handleSection()
{
  QString tempSection, tempSectionSuffix;

  if(currentLine.at(currentLine.size() - 1) == ']')
    tempSection = currentLine.mid(1, currentLine.size() - 2);
  else
  {
    tempSection = currentLine.mid(1, currentLine.size() - 1);
    qWarning() << "Missing closing \"]\":" << currentLine;
  }

  int dotPos = tempSection.indexOf('.');
  if(dotPos >= 0)
  {
    QString sectPrefix = tempSection.mid(0, dotPos).toLower();
    if(sectPrefix.isEmpty())
      qWarning() << "Missing section name before \".\":" << currentLine;

    tempSectionSuffix = tempSection.mid(dotPos + 1).toLower();
    if(tempSectionSuffix.isEmpty())
      qWarning() << "Missing section suffix after \".\":" << currentLine;

    tempSection = sectPrefix;
  }

  if(!currentSection.isEmpty())
    onEndSection(currentSection, currentSectionSuffix);

  currentSection = tempSection.toLower();
  currentSectionSuffix = tempSectionSuffix.toLower();
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

    if(!codec.isEmpty())
      sceneryCfg.setCodec(codec.toLatin1().constData());

    onStartDocument(filename);

    currentLine.clear();
    while(sceneryCfg.readLineInto(&currentLine, 1024))
    {
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

    if(!currentSection.isEmpty())
      onEndSection(currentSection, currentSectionSuffix);

    onEndDocument(filename);
    sceneryCfgFile.close();
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(iniFilename).arg(sceneryCfgFile.errorString()));
}

void IniReader::throwException(const QString& message)
{
  throw Exception(tr("%1. File \"%2\", line %3:\"%4\"").
                  arg(message).arg(filename).arg(currentLineNum).arg(currentLine));
}

bool IniReader::toBool(const QString& str)
{
  QString tmp = str.toLower().trimmed();

  if(tmp == "true" || tmp == "t" || tmp == "y" || tmp == "yes" || tmp == "1")
    return true;
  else if(tmp == "false" || tmp == "f" || tmp == "n" || tmp == "no" || tmp == "0")
    return false;

  qWarning() << "Boolean value not valid in scenery area line" << currentLineNum << "file" << filename;
  return false;
}

int IniReader::toInt(const QString& str)
{
  int retval = 0;
  bool ok = false;
  retval = str.toInt(&ok);
  if(!ok)
    qWarning() << "Int value not valid in scenery area line" << currentLineNum << "file" << filename;
  return retval;
}

} // namespace io
} // namespace atools
