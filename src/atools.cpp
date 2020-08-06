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
#include <QStandardPaths>

namespace atools {

const static QChar SEP(QDir::separator());

QString version()
{
  return "3.6.0.beta"; // VERSION_NUMBER - atools
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

QStringList probeFile(const QString& file, int numLinesRead)
{
  QFile testFile(file);

  QStringList lines;
  if(testFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&testFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);

    int numLines = 0, numLinesTotal = 0;
    while(!stream.atEnd() && numLines < numLinesRead && numLinesTotal < numLinesRead * 2)
    {
      QString line = stream.readLine(256).trimmed();
      if(!line.isEmpty())
      {
        lines.append(line.toLower().simplified());
        numLines++;
      }
      numLinesTotal++;
    }

    // Fill missing entries with empty strings to ease checking.
    for(int i = lines.size(); i < 6; i++)
      lines.append(QString());
    testFile.close();
  }
  else
    throw Exception(QObject::tr("Error reading \"%1\": %2").arg(file).arg(testFile.errorString()));

  return lines;
}

QString capWord(QString str)
{
  if(!str.isEmpty())
    str.replace(0, 1, str.at(0).toUpper());
  return str;
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
  return QString(QObject::tr("★", "Star for rating")).repeated(value) + QString(QObject::tr("−", "For empty rating")).
         repeated(maxValue - value);
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
  return QString(filename).replace('\\', ' ').replace('/', ' ').replace(':', ' ').replace('\'', ' ').replace('\"', ' ').
         replace('*', ' ').replace('<', ' ').replace('>', ' ').replace('?', ' ').replace('$', ' ').simplified();
}

bool strContains(const QString& name, const std::initializer_list<QString>& list)
{
  for(const QString& val : list)
    if(name.contains(val))
      return true;

  return false;
}

bool strContains(const QString& name, const std::initializer_list<const char *>& list)
{
  for(const char *val : list)
    if(name.contains(val))
      return true;

  return false;
}

bool strContains(const QString& name, const std::initializer_list<char>& list)
{
  for(char val : list)
    if(name.contains(val))
      return true;

  return false;
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

  // Use the same for macOS since case sensitive file systems can cause problems
#if defined(Q_OS_WIN32)
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
        if(QFileInfo(dir.path() + SEP + entries.first()).isDir())
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
        dir = dir.path() + SEP + path;
    }
    i++;
  }

  if(file.isEmpty())
    return dir.path();
  else
    return dir.path() + SEP + file;

#endif
}

QString buildPath(const QStringList& paths)
{
  QString retval;

  int i = 0;
  for(const QString& path : paths)
  {
    if(i > 0)
      retval += SEP;
    retval += path;
    i++;
  }
  return retval;
}

QString blockText(const QStringList& texts, int maxItemsPerLine, const QString& itemSeparator,
                  const QString& lineSeparator)
{
  // Convert long list of items to blocks
  QVector<QStringList> blocks;
  blocks.append(QStringList());

  for(const QString& str : texts)
  {
    if(blocks.last().size() >= maxItemsPerLine)
      blocks.append(QStringList());
    blocks.last().append(str);
  }

  // Join items by , and blocks by linefeed
  QString txt;
  for(const QStringList& list : blocks)
    txt.append((txt.isEmpty() ? QString() : itemSeparator + lineSeparator) + list.join(itemSeparator));
  return txt;
}

QString elideTextShort(const QString& str, int maxLength)
{
  if(str.size() > maxLength)
    return str.left(maxLength - 1) + QObject::tr("…", "Dots used to shorten texts");

  return str;
}

QString elideTextShortMiddle(const QString& str, int maxLength)
{
  if(maxLength / 2 + maxLength / 2 + 1 < str.size()) // Avoid same size replacement due to round down
    return str.left(maxLength / 2) + QObject::tr("…", "Dots used to shorten texts") + str.right(maxLength / 2);

  return str;
}

QString elideTextLinesShort(QString str, int maxLines, int maxLength)
{
  QStringList lines;
  QTextStream stream(&str, QIODevice::ReadOnly);

  int i = 0;
  while(!stream.atEnd() && ++i < maxLines)
    lines.append(maxLength > 0 ? elideTextShort(stream.readLine(), maxLength) : stream.readLine());

  if(i >= maxLines)
    return lines.join(QObject::tr("\n", "Linefeed used to shorten large texts")) +
           QObject::tr("\n…", "Linefeed and dots used to shorten texts");
  else
    return lines.join(QObject::tr("\n", "Linefeed used to shorten large texts"));
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

QString at(const QStringList& columns, int index, bool error)
{
  if(index < columns.size())
    return columns.at(index).trimmed();
  else if(error)
    qWarning() << "Invalid index" << index << "for" << columns;
  return QString();
}

int atInt(const QStringList& columns, int index, bool error)
{
  int num = 0;
  QString str = at(columns, index).trimmed();
  if(!str.isEmpty())
  {
    bool ok;
    num = str.toInt(&ok);
    if(!ok && error)
      qWarning() << "Invalid number" << str << "at" << index << "for" << columns;
  }
  return num;
}

float atFloat(const QStringList& columns, int index, bool error)
{
  float num = 0.f;
  QString str = at(columns, index).trimmed();
  if(!str.isEmpty())
  {
    bool ok;
    num = str.toFloat(&ok);
    if(!ok && error)
      qWarning() << "Invalid floating point number" << str << "at" << index << "for" << columns;
  }
  return num;
}

QDateTime atDateTime(const QStringList& columns, int index, bool error)
{
  return QDateTime::fromString(at(columns, index, error), Qt::ISODate);
}

QTime timeFromHourMinStr(const QString& timeStr)
{
  QTime time;
  bool okHours = true, okMinutes = true;
  if(timeStr.contains(":"))
    time = QTime(timeStr.section(':', 0, 0).toInt(&okHours), timeStr.section(':', 1, 1).toInt(&okMinutes));
  else if(timeStr.length() == 3 || timeStr.length() == 4)
    time = QTime(timeStr.left(timeStr.length() - 2).toInt(&okHours), timeStr.right(2).toInt(&okMinutes));

  return !okHours || !okMinutes ? QTime() : time;
}

QString strFromFile(const QString& filename)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    return stream.readAll();
  }
  return QString();
}

QString homeDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
}

QString desktopDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first();
}

QString documentsDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
}

QString downloadDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first();
}

QString tempDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::TempLocation).first();
}

QStringList numberVectorToStrList(const QVector<int>& vector)
{
  QStringList retval;
  for(int value : vector)
    retval.append(QString::number(value));
  return retval;
}

QVector<int> strListToNumberVector(const QStringList& strings, bool *ok)
{
  if(ok != nullptr)
    *ok = true;
  QVector<int> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    int val = str.toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.append(val);
  }
  return retval;
}

QStringList numberSetToStrList(const QSet<int>& set)
{
  QStringList retval;
  for(int value : set)
    retval.append(QString::number(value));
  return retval;
}

QSet<int> strListToNumberSet(const QStringList& strings, bool *ok)
{
  if(ok != nullptr)
    *ok = true;
  QSet<int> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    int val = str.toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val);
  }
  return retval;
}

QStringList numberStrHashToStrList(const QHash<int, QString>& hash)
{
  QStringList retval;

  for(auto i = hash.begin(); i != hash.end(); ++i)
  {
    retval.append(QString::number(i.key()));
    retval.append(i.value());
  }
  return retval;
}

QHash<int, QString> strListToNumberStrHash(const QStringList& strings, bool *ok)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QHash<int, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    int val = strings.at(i).toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val, strings.at(i + 1));
  }
  return retval;
}

QStringList numberStrMapToStrList(const QMap<int, QString>& map)
{
  QStringList retval;

  for(auto i = map.begin(); i != map.end(); ++i)
  {
    retval.append(QString::number(i.key()));
    retval.append(i.value());
  }
  return retval;
}

QMap<int, QString> strListToNumberStrMap(const QStringList& strings, bool *ok)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QMap<int, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    int val = strings.at(i).toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val, strings.at(i + 1));
  }
  return retval;
}

QString strJoin(const QStringList& list, const QString& sep)
{
  QString retval;
  for(QString str : list)
  {
    str = str.trimmed();
    if(!str.isEmpty())
    {
      if(!retval.isEmpty())
        retval += sep;
      retval += str;
    }
  }
  return retval;
}

QString strJoin(const QStringList& list, const QString& sep, const QString& lastSep, const QString& suffix)
{
  QString retval;

  QStringList listTrimmed;
  for(QString str : list)
  {
    str = str.trimmed();
    if(!str.isEmpty())
      listTrimmed.append(str);
  }

  for(int i = 0; i < listTrimmed.size(); i++)
  {
    QString str = listTrimmed.at(i);
    if(!retval.isEmpty())
      retval += (i >= listTrimmed.size() - 1 ? lastSep : sep);
    retval += str;
  }

  if(!retval.isEmpty())
    retval.append(suffix);

  return retval;
}

QString buildPathNoCase(QString path)
{
  return buildPathNoCase(path.replace('\\', '/').split('/'));
}

} // namespace atools
