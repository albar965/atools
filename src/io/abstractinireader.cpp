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

#include "io/abstractinireader.h"
#include "exception.h"

#include <QDebug>
#include <QTextStream>
#include <QIODevice>
#include <QFile>

namespace atools {
namespace io {

AbstractIniReader::AbstractIniReader(const QString& textCodec)
  : currentLineNum(0), codec(textCodec), commentCharacters(";")
{
}

AbstractIniReader::~AbstractIniReader()
{
}

void AbstractIniReader::handleComment()
{
  for(const QString& comment : commentCharacters)
  {
    int c = currentLine.indexOf(comment);
    if(c >= 0)
      currentLine.truncate(c);
  }
}

void AbstractIniReader::handleKeyValue()
{
  QString name, value;

  int c = currentLine.indexOf('=');
  if(c >= 0)
  {
    name = changeCase(currentLine.left(c));
    if(name.isEmpty())
      qWarning() << "Missing key name before \"=\":" << currentLine;
    else
    {
      value = currentLine.mid(c + 1);

      // Call both - implementor can choose to ignore one
      onKeyValue(currentSection, currentSectionSuffix, name, value);
      onKeyValue(currentSection + (currentSectionSuffix.isEmpty() ?
                                   QString() : "." + currentSectionSuffix), name, value);
    }
  }
  else
    qWarning() << "Missing \"=\":" << currentLine;
}

void AbstractIniReader::handleSection()
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
    QString sectPrefix = changeCase(tempSection.mid(0, dotPos));
    if(sectPrefix.isEmpty())
      qWarning() << "Missing section name before \".\":" << currentLine;

    tempSectionSuffix = changeCase(tempSection.mid(dotPos + 1));
    if(tempSectionSuffix.isEmpty())
      qWarning() << "Missing section suffix after \".\":" << currentLine;

    tempSection = sectPrefix;
  }

  if(!currentSection.isEmpty())
    onEndSection(currentSection, currentSectionSuffix);

  currentSection = changeCase(tempSection);
  currentSectionSuffix = changeCase(tempSectionSuffix);
  onStartSection(currentSection, currentSectionSuffix);
}

void AbstractIniReader::read(const QString& iniFilename)
{
  currentSection.clear();
  currentSectionSuffix.clear();
  currentLine.clear();
  currentLineNum = 0;

  this->filepath = iniFilename;

  QFile sceneryCfgFile(filepath);

  if(sceneryCfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream sceneryCfg(&sceneryCfgFile);

    if(!codec.isEmpty())
      sceneryCfg.setCodec(codec.toLatin1().constData());

    onStartDocument(filepath);

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

    onEndDocument(filepath);
    sceneryCfgFile.close();
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(iniFilename).arg(sceneryCfgFile.errorString()));
}

void AbstractIniReader::onStartDocument(const QString& filename)
{
  Q_UNUSED(filename)
}

void AbstractIniReader::onEndDocument(const QString& filename)
{
  Q_UNUSED(filename)
}

void AbstractIniReader::onStartSection(const QString& name, const QString& nameSuffix)
{
  Q_UNUSED(name)
  Q_UNUSED(nameSuffix)
}

void AbstractIniReader::onEndSection(const QString& section, const QString& sectionSuffix)
{
  Q_UNUSED(section)
  Q_UNUSED(sectionSuffix)
}

void AbstractIniReader::onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                                   const QString& value)
{
  Q_UNUSED(section)
  Q_UNUSED(sectionSuffix)
  Q_UNUSED(key)
  Q_UNUSED(value)
}

void AbstractIniReader::onKeyValue(const QString& section, const QString& key, const QString& value)
{
  Q_UNUSED(section)
  Q_UNUSED(key)
  Q_UNUSED(value)
}

void AbstractIniReader::throwException(const QString& message)
{
  throw Exception(tr("%1. File \"%2\", line %3:\"%4\"").
                  arg(message).arg(filepath).arg(currentLineNum).arg(currentLine));
}

bool AbstractIniReader::toBool(const QString& str)
{
  QString tmp = str.toLower().trimmed();

  if(tmp == "true" || tmp == "t" || tmp == "y" || tmp == "yes" || tmp == "1")
    return true;
  else if(tmp == "false" || tmp == "f" || tmp == "n" || tmp == "no" || tmp == "0")
    return false;

  qWarning() << "Boolean value not valid in scenery area line" << currentLineNum << "file" << filepath;
  return false;
}

int AbstractIniReader::toInt(const QString& str)
{
  int retval = 0;
  bool ok = false;
  retval = str.toInt(&ok);
  if(!ok)
    qWarning() << "Int value not valid in scenery area line" << currentLineNum << "file" << filepath;
  return retval;
}

QString AbstractIniReader::changeCase(const QString& str)
{
  return preserveCase ? str : str.toLower();
}

} // namespace io
} // namespace atools
