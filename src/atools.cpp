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

#include "atools.h"
#include "exception.h"

#include <QDebug>
#include <QLocale>
#include <QRegularExpression>
#include <QVector>
#include <QDir>
#include <QTextCodec>
#include <QCoreApplication>
#include <QDateTime>

namespace atools {

QString version()
{
  return "3.1.0.develop"; // VERSION_NUMBER
}

QString gitRevision()
{
  return GIT_REVISION_ATOOLS;
}

QTextCodec *codecForFile(QFile& file, QTextCodec *defaultCodec)
{
  QTextCodec *codec = nullptr;
  file.seek(0);

  // Load a part of the file and detect the BOM/codec
  const qint64 PROBE_SIZE = 128;
  char *buffer = new char[PROBE_SIZE];
  qint64 bytesRead = file.read(buffer, PROBE_SIZE);
  if(bytesRead > 0)
    codec = QTextCodec::codecForUtfText(QByteArray(buffer, static_cast<int>(bytesRead)), defaultCodec);
  delete[] buffer;

  file.seek(0);
  return codec;
}

void capWord(QString& lastWord, QChar last, const QSet<QString>& toUpper,
             const QSet<QString>& toLower, const QSet<QString>& ignore)
{
  static QLocale locale;
  if(toUpper.contains(lastWord.toUpper()))
    lastWord = locale.toUpper(lastWord);
  else if(toLower.contains(lastWord))
    lastWord = locale.toLower(lastWord.toLower());
  else if(!ignore.contains(lastWord))
  {
    // Convert all letters after an apostrophe to lower case (St. Mary's)
    if(last == '\'' && lastWord.size() == 1)
      lastWord[0] = lastWord.at(0).toLower();
    else
    {
      lastWord = lastWord.toLower();
      lastWord[0] = lastWord.at(0).toUpper();
    }
  }
}

QString capString(const QString& str, const QSet<QString>& toUpper, const QSet<QString>& toLower,
                  const QSet<QString>& ignore)
{
  if(str.isEmpty())
    return str;

  QString retval, lastWord;
  QChar last, lastSep;
  for(QChar c : str)
  {
    if(!c.isLetterOrNumber())
    {
      if(last.isLetterOrNumber())
      {
        capWord(lastWord, lastSep, toUpper, toLower, ignore);
        retval += lastWord;
        lastWord.clear();
      }
      retval += c;
      lastSep = c;
    }
    else
      lastWord += c;

    last = c;
  }
  if(!lastWord.isEmpty())
  {
    QChar lastC = str.at(str.size() >= 2 ? str.size() - 2 : 0);
    capWord(lastWord, lastC, toUpper, toLower, ignore);
    retval += lastWord;
  }

  return retval.replace('_', ' ');
}

QString ratingString(int value, int maxValue)
{
  return QString("★").repeated(value) + QString("−").repeated(maxValue - value);
}

QString replaceVar(QString str, const QString& name, const QVariant& value)
{
  QHash<QString, QVariant> variableValues;
  variableValues.insert(name, value);
  return replaceVar(str, variableValues);
}

QString replaceVar(QString str, const QHash<QString, QVariant>& variableValues)
{
  QString retval(str);

  for(const QString& variable : variableValues.keys())
    retval.replace(QRegularExpression("\\$\\{" + variable + "\\}"),
                   variableValues.value(variable).toString());

  return retval;
}

QString cleanFilename(const QString& filename)
{
  return QString(filename).replace('\\', ' ').replace('/', ' ').replace(':', ' ').replace('\'', ' ').
         replace('*', ' ').replace('<', ' ').replace('>', ' ').replace('?', ' ').replace('$', ' ').
         replace("  ", " ");
}

bool contains(const QString& name, const std::initializer_list<QString>& list)
{
  for(const QString& val : list)
    if(val == name)
      return true;

  return false;
}

bool contains(const QString& name, const std::initializer_list<const char *>& list)
{
  for(const char *val : list)
    if(val == name)
      return true;

  return false;
}

QString buildPathNoCase(const QStringList& paths)
{

#if defined(Q_OS_WIN32) || defined(Q_OS_MACOS)
  return buildPath(paths);

#else
  QDir dir;
  QString file;

  int i = 0;
  for(const QString& path : paths)
  {
    if(i == 0)
      // First path element
      dir = path;
    else
    {
      // Get entries that match exacly the next path element
      QStringList entries = dir.entryList({path});

      if(entries.isEmpty())
      {
        // Nothing found - do an expensive manual compare
        bool found = false;
        entries = dir.entryList();

        for(const QString& str: entries)
        {
          if(str.compare(path, Qt::CaseInsensitive) == 0)
          {
            // Found something - use it as the single entry
            entries.clear();
            entries.append(str);
            found = true;
            break;
          }
        }

        if(!found)
          // Nothing found when searching case insensitive
          entries.clear();
      }

      if(!entries.isEmpty())
      {
        if(QFileInfo(dir.path() + QDir::separator() + entries.first()).isDir())
        {
          // Directory exists - change into it
          if(!dir.cd(entries.first()))
            break;
        }
        else
        {
          // Is a file - add by name simply
          file = entries.first();
          break;
        }
      }
      else
        // Nothing found - add potentially wrong case name
        dir = dir.path() + QDir::separator() + path;
    }
    i++;
  }

  if(file.isEmpty())
    return dir.path();
  else
    return dir.path() + QDir::separator() + file;

#endif
}

QString buildPath(const QStringList& paths)
{
  QString retval;

  int i = 0;
  for(const QString& path : paths)
  {
    if(i > 0)
      retval += QDir::separator();
    retval += path;
    i++;
  }
  return retval;
}

QString elideTextShort(const QString& str, int maxLength)
{
  if(str.size() > maxLength)
    return str.left(maxLength - 1) + "…";

  return str;
}

QString elideTextLinesShort(QString str, int maxLength)
{
  QStringList lines;
  QTextStream stream(&str, QIODevice::ReadOnly);

  int i = 0;
  while(!stream.atEnd() && ++i < maxLength)
    lines.append(stream.readLine());

  if(i >= maxLength)
    return lines.join("\n") + "\n…";
  else
    return lines.join("\n");
}

float calculateSteps(float range, float numSteps)
{
  float a = range;
  float step = numSteps;
  float mag = std::pow(10.f, std::floor(std::log10(a / step)));
  float val = std::floor((a / step) / mag);

  if(val < 1.f)
    val = 1.f;
  else if(val < 2.f)
    val = 2.f;
  else if(val < 5.f)
    val = 5.f;
  else
    val = 10.f;

  return val * mag;
}

QString programFileInfo()
{
  return QObject::tr("Created by %1 Version %2 (revision %3) on %4").
         arg(QCoreApplication::applicationName()).
         arg(QCoreApplication::applicationVersion()).
         arg(atools::gitRevision()).
         arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
         replace("-", " ");
}

QString programFileInfoNoDate()
{
  return QObject::tr("Created by %1 Version %2 (revision %3)").
         arg(QCoreApplication::applicationName()).
         arg(QCoreApplication::applicationVersion()).
         arg(atools::gitRevision()).
         replace("-", " ");
}

bool fileEndsWithEol(const QString& filepath)
{
  bool endsWithEol = false;
  QFile tmp(filepath);
  if(!tmp.exists())
    // No file - no need to add extra EOL
    return true;

  if(tmp.open(QFile::ReadOnly))
  {
    tmp.seek(tmp.size() - 1);
    char lastChar = '\0';
    tmp.read(&lastChar, 1);
    tmp.close();

    endsWithEol = lastChar == '\n' || lastChar == '\r';
  }
  else
    throw atools::Exception(QObject::tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(tmp.errorString()));
  return endsWithEol;
}

QString at(const QStringList& columns, int index)
{
  if(index < columns.size())
    return columns.at(index).trimmed();
  else
    qWarning() << "Invalid index" << index << "for" << columns;
  return QString();
}

int atInt(const QStringList& columns, int index)
{
  int num = 0;
  QString str = at(columns, index).trimmed();
  if(!str.isEmpty())
  {
    bool ok;
    num = str.toInt(&ok);
    if(!ok)
      qWarning() << "Invalid number" << str << "at" << index << "for" << columns;
  }
  return num;
}

float atFloat(const QStringList& columns, int index)
{
  float num = 0.f;
  QString str = at(columns, index).trimmed();
  if(!str.isEmpty())
  {
    bool ok;
    num = str.toFloat(&ok);
    if(!ok)
      qWarning() << "Invalid floating point number" << str << "at" << index << "for" << columns;
  }
  return num;
}

} // namespace atools
