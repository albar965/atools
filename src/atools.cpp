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
#include <QFontMetrics>

namespace atools {

const static QChar SEP(QDir::separator());

QString version()
{
  return "3.8.2"; // VERSION_NUMBER - atools
}

QString gitRevision()
{
  return GIT_REVISION_ATOOLS;
}

QTextCodec *codecForFile(QIODevice& file, QTextCodec *defaultCodec)
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

  for(auto it = variableValues.constBegin(); it != variableValues.constEnd(); ++it)
    retval.replace(QRegularExpression("\\$\\{" + it.key() + "\\}"), it.value().toString());

  return retval;
}

// \\  /  :  \'  *  &amp;  &gt;  &lt;  ?  $  |
static const QString INVALID_FILENAME_CHARACTERS("\\/:\'\"*<>?$|");

QString invalidFilenameCharacters(bool html)
{
  QStringList retval;

  for(QChar c : INVALID_FILENAME_CHARACTERS)
  {
    QString str(c);
    if(html)
      retval.append(str.toHtmlEscaped());
    else
      retval.append(str);
  }

  if(html)
    return retval.join("&nbsp;&nbsp;");
  else
    return retval.join("  ");
}

QString cleanFilename(QString filename, int maxLength)
{
  for(QChar c : INVALID_FILENAME_CHARACTERS)
    filename.replace(c, ' ');

  return filename.simplified().mid(0, maxLength);
}

bool strContains(const QString& name, const QStringList& list)
{
  for(const QString& val : list)
    if(name.contains(val))
      return true;

  return false;
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

QString blockText(const QStringList& texts, int maxItemsPerLine, const QString& itemSeparator,
                  const QString& lineSeparator)
{
  // Convert long list of items to blocks
  QVector<QStringList> blocks;
  blocks.append(QStringList());

  for(const QString& str : texts)
  {
    if(blocks.constLast().size() >= maxItemsPerLine)
      blocks.append(QStringList());
    blocks.last().append(str);
  }

  // Join items by , and blocks by linefeed
  QString txt;
  for(const QStringList& list : blocks)
    txt.append((txt.isEmpty() ? QString() : itemSeparator + lineSeparator) + list.join(itemSeparator));
  return txt;
}

QStringList elideTextShort(QStringList list, int maxLength)
{
  for(QString& str : list)
    str = elideTextShort(str, maxLength);

  return list;
}

QString elideTextShort(const QString& str, int maxLength)
{
  if(str.size() > maxLength)
    return str.left(maxLength - 1) + QObject::tr("…", "Dots used to shorten texts");

  return str;
}

QString elideTextShortLeft(const QString& str, int maxLength)
{
  if(str.size() > maxLength)
    return QObject::tr("…", "Dots used to shorten texts") + str.right(maxLength - 1);

  return str;
}

QString elideTextShortMiddle(const QString& str, int maxLength)
{
  if(maxLength / 2 + maxLength / 2 + 1 < str.size()) // Avoid same size replacement due to round down
    return str.left(maxLength / 2) + QObject::tr("…", "Dots used to shorten texts") + str.right(maxLength / 2);

  return str;
}

QString elideTextLinesShort(QString str, int maxLines, int maxLength, bool compressEmpty, bool ellipseLastLine)
{
  QStringList lines;
  QTextStream stream(&str, QIODevice::ReadOnly);

  int i = 0;
  while(!stream.atEnd())
  {
    QString line = stream.readLine();
    if(compressEmpty)
    {
      line = line.simplified();
      if(line.isEmpty())
        continue;
    }

    if(i++ >= maxLines)
      break;

    lines.append(maxLength > 0 ? elideTextShort(line, maxLength) : line);
  }

  if(i > maxLines)
    return lines.join(QObject::tr("\n", "Linefeed used to shorten large texts")) +
           (ellipseLastLine ?
            QObject::tr("\n…", "Linefeed and dots used to shorten texts") :
            QObject::tr("…", "Linefeed and dots used to shorten texts"));
  else
    return lines.join(QObject::tr("\n", "Linefeed used to shorten large texts"));
}

QStringList elidedTexts(const QFontMetrics& metrics, const QStringList& texts, Qt::TextElideMode mode, int width)
{
  QStringList retval(texts);
  for(QString& str : retval)
    str = elidedText(metrics, str, mode, width);
  return retval;
}

QString elidedText(const QFontMetrics& metrics, QString text, Qt::TextElideMode mode, int width)
{
  if(metrics.horizontalAdvance(text) >= width)
  {
    text = metrics.elidedText(text, mode, width);
    if(text.length() < 3)
    {
      QString dots(QObject::tr("…", "Dots used to shorten texts"));
      QString dot(QObject::tr(".", "Dot used to shorten texts"));

      if(metrics.horizontalAdvance(dots) < width)
        return dots;
      else if(metrics.horizontalAdvance(dot) < width)
        return dot;
    }
  }

  return text;
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
    time = QTime(timeStr.left(timeStr.length() - 2).toInt(&okHours), timeStr.rightRef(2).toInt(&okMinutes));

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
  return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
}

QString desktopDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0);
}

QString documentsDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0);
}

QString downloadDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).at(0);
}

QString tempDir()
{
  return QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
}

QStringList floatVectorToStrList(const QVector<float>& vector)
{
  QStringList retval;
  for(float value : vector)
    retval.append(QString::number(value));
  return retval;
}

QVector<float> strListToFloatVector(const QStringList& strings, bool *ok)
{
  if(ok != nullptr)
    *ok = true;
  QVector<float> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    float val = str.toFloat(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.append(val);
  }
  return retval;
}

QStringList floatSetToStrList(const QSet<float>& set)
{
  QStringList retval;
  for(float value : set)
    retval.append(QString::number(value));
  return retval;
}

QSet<float> strListToFloatSet(const QStringList& strings, bool *ok)
{
  if(ok != nullptr)
    *ok = true;
  QSet<float> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    float val = str.toFloat(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val);
  }
  return retval;
}

QStringList floatStrHashToStrList(const QHash<float, QString>& hash)
{
  QStringList retval;

  for(auto i = hash.constBegin(); i != hash.constEnd(); ++i)
  {
    retval.append(QString::number(i.key()));
    retval.append(i.value());
  }
  return retval;
}

QHash<float, QString> strListToFloatStrHash(const QStringList& strings, bool *ok)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QHash<float, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    float val = strings.at(i).toFloat(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val, strings.at(i + 1));
  }
  return retval;
}

QStringList floatStrMapToStrList(const QMap<float, QString>& map)
{
  QStringList retval;

  for(auto i = map.constBegin(); i != map.constEnd(); ++i)
  {
    retval.append(QString::number(i.key()));
    retval.append(i.value());
  }
  return retval;
}

QMap<float, QString> strListToFloatStrMap(const QStringList& strings, bool *ok)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QMap<float, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    float val = strings.at(i).toFloat(&localOk);

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

QString strJoin(const QString& prefix, const QStringList& list, const QString& sep, const QString& lastSep, const QString& suffix)
{
  QString retval = strJoin(list, sep, lastSep, suffix);
  if(!retval.isEmpty())
    retval.prepend(prefix);
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

QString buildPath(const QStringList& paths)
{
  return paths.join(SEP);
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
      dir.setPath(path);
    else
    {
      // Get entries that match exacly the next path element
      QStringList entries = dir.entryList({path});

      if(entries.isEmpty())
      {
        // Nothing found - do an expensive manual compare
        bool found = false;
        entries = dir.entryList();

        for(const QString& str : entries)
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
        if(QFileInfo(dir.path() + SEP + entries.constFirst()).isDir())
        {
          // Directory exists - change into it
          if(!dir.cd(entries.constFirst()))
            break;
        }
        else
        {
          // Is a file - add by name simply
          file = entries.constFirst();
          break;
        }
      }
      else
        // Nothing found - add potentially wrong case name
        dir.setPath(dir.path() + SEP + path);
    }
    i++;
  }

  if(file.isEmpty())
    return dir.path();
  else
    return dir.path() + SEP + file;

#endif
}

QString checkDirMsg(const QString& dir, int maxLength, bool warn)
{
  return checkDirMsg(QFileInfo(dir), maxLength, warn);
}

QString checkDirMsg(const QFileInfo& dir, int maxLength, bool warn)
{
  if(dir.filePath().isEmpty())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "Dir is empty";
    return QObject::tr("Directory name is empty.");
  }
  else
  {
    QString shortName = elideTextShortLeft(dir.absoluteFilePath(), maxLength);
    if(!dir.exists())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "Directory" << dir.absoluteFilePath() << "does not exist";
      return QObject::tr("Directory \"%1\" does not exist.").arg(shortName);
    }
    else
    {
      if(!dir.isDir())
      {
        if(warn)
          qWarning() << Q_FUNC_INFO << "File" << dir.absoluteFilePath() << "is not a directory";
        return QObject::tr("File \"%1\" is not a directory.").arg(shortName);
      }
      else if(!dir.isReadable())
      {
        if(warn)
          qWarning() << Q_FUNC_INFO << "Directory" << dir.absoluteFilePath() << "is not readable";
        return QObject::tr("Directory \"%1\" is not readable.").arg(shortName);
      }
    }
  }
  return QString();
}

QString checkFileMsg(const QString& file, int maxLength, bool warn)
{
  return checkFileMsg(QFileInfo(file), maxLength, warn);
}

QString checkFileMsg(const QFileInfo& file, int maxLength, bool warn)
{
  if(file.filePath().isEmpty())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "Filepath is empty";
    return QObject::tr("Filepath is empty.");
  }
  else
  {
    QString shortName = elideTextShortLeft(file.absoluteFilePath(), maxLength);
    if(!file.exists())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "does not exist";
      return QObject::tr("File \"%1\" does not exist.").arg(shortName);
    }
    else
    {
      if(!file.isFile())
      {
        if(warn)
          qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is a directory";
        return QObject::tr("File \"%1\" is a directory.").arg(shortName);
      }
      else if(!file.isReadable())
      {
        qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is not readable";
        return QObject::tr("File \"%1\" is not readable.").arg(shortName);
      }
      else if(file.size() == 0)
      {
        if(warn)
          qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is empty";
        return QObject::tr("File \"%1\" is empty.").arg(shortName);
      }
    }
  }
  return QString();
}

bool checkDir(const QString& dir, bool warn)
{
  return checkDir(QFileInfo(dir), warn);
}

bool checkDir(const QFileInfo& dir, bool warn)
{
  if(dir.filePath().isEmpty())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "Dir is empty";
    return false;
  }
  else if(!dir.exists())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "Directory" << dir.absoluteFilePath() << "does not exist";
    return false;
  }
  else
  {
    if(!dir.isDir())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "File" << dir.absoluteFilePath() << "is not a directory";
      return false;
    }
    else if(!dir.isReadable())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "Directory" << dir.absoluteFilePath() << "is not readable";
      return false;
    }
  }
  return true;
}

bool checkFile(const QString& file, bool warn)
{
  return checkFile(QFileInfo(file), warn);
}

bool checkFile(const QFileInfo& file, bool warn)
{
  if(file.filePath().isEmpty())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "Filepath is empty";
    return false;
  }
  else if(!file.exists())
  {
    if(warn)
      qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "does not exist";
    return false;
  }
  else
  {
    if(!file.isFile())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is a directory";
      return false;
    }
    else if(!file.isReadable())
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is not readable";
      return false;
    }
    else if(file.size() == 0)
    {
      if(warn)
        qWarning() << Q_FUNC_INFO << "File" << file.absoluteFilePath() << "is empty";
      return false;
    }
  }
  return true;
}

bool strStartsWith(const QStringList& list, const QString& str)
{
  for(const QString& s : list)
  {
    if(str.startsWith(s))
      return true;
  }

  return false;
}

bool strAnyStartsWith(const QStringList& list, const QString& str)
{
  for(const QString& s : list)
  {
    if(s.startsWith(str))
      return true;
  }

  return false;
}

QString removeNonPrintable(const QString& str)
{
  QString trimmed;
  for(QChar c : str)
  {
    if(c.isPrint())
      trimmed.append(c);
  }
  return trimmed;
}

QString removeNonAlphaNum(const QString& str)
{
  QString trimmed;
  for(QChar c : str)
  {
    if(c.isLetterOrNumber() || c.isPunct())
      trimmed.append(c);
  }
  return trimmed;
}

QString normalizeStr(const QString& str)
{
  // Decompose string into base characters and diacritics
  QString retval, norm = str.normalized(QString::NormalizationForm_KD);
  for(QChar c : norm)
  {
    // Remove diacritics
    if(c.category() != QChar::Mark_NonSpacing)
    {
      if(c == '?')
        // Native question mark - keep
        retval.append(c);
      else
      {
        // Convert ot latin
        c = c.toLatin1();

        // Add only if latin conversion did not produce garbage
        if(c != '?' && c.isPrint() && c.unicode() < 126)
          retval.append(c);
      }
    }
  }
  return retval;
}

QDateTime correctDate(int day, int hour, int minute)
{
  QDateTime dateTime = QDateTime::currentDateTimeUtc();
  dateTime.setDate(QDate(dateTime.date().year(), dateTime.date().month(), day));
  dateTime.setTime(QTime(hour, minute));

  // Keep subtracting months until it is not in the future and the day matches
  // but not more than one year to avoid endless loops
  int months = 0;
  while((dateTime > QDateTime::currentDateTimeUtc() || day != dateTime.date().day()) && months < 12)
    dateTime = dateTime.addMonths(-(++months));
  return dateTime;
}

QDateTime correctDateLocal(int dayOfYear, int secondsOfDayLocal, int secondsOfDayUtc)
{
  QDate localDate = QDate(QDate::currentDate().year(), 1, 1).addDays(dayOfYear);

  int offsetSeconds = 0;
  if(secondsOfDayLocal - secondsOfDayUtc <= -12 * 3600)
    // UTC is one day back
    offsetSeconds = secondsOfDayLocal - secondsOfDayUtc + 24 * 3600;
  else if(secondsOfDayLocal - secondsOfDayUtc >= 12 * 3600)
    // UTC is one day forward
    offsetSeconds = secondsOfDayLocal - secondsOfDayUtc - 24 * 3600;
  else
    // UTC is same day as local
    offsetSeconds = secondsOfDayLocal - secondsOfDayUtc;

  return QDateTime(localDate, QTime::fromMSecsSinceStartOfDay(secondsOfDayLocal * 1000),
                   Qt::OffsetFromUTC, offsetSeconds);
}

QDateTime timeToNextHourInterval(QDateTime datetime, int intervalsPerDay)
{
  datetime = timeToLastHourInterval(datetime, intervalsPerDay);
  datetime = datetime.addSecs(intervalsPerDay * 3600);
  return datetime;
}

QDateTime timeToLastHourInterval(QDateTime datetime, int intervalsPerDay)
{
  int hour = datetime.time().hour();
  int down = hour - (hour / intervalsPerDay * intervalsPerDay);
  datetime.setTime(QTime(hour, 0, 0));
  datetime = datetime.addSecs(-down * 3600);
  return datetime;
}

uint textFileHash(const QString& filename, const QString& codec)
{
  QFile file(filename);
  QByteArray latin1 = codec.toLatin1();
  uint hash = 0;

  if(file.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&file);
    stream.setCodec(latin1.constData());
    stream.setAutoDetectUnicode(true);

    while(!stream.atEnd())
    {
      QString line = stream.readLine().trimmed();
      if(!line.isEmpty())
        hash ^= qHash(line, 97);
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "Error reading" << filename << file.errorString();

  return hash;
}

QString convertToIsoWithOffset(const QDateTime& dateTime, bool milliseconds)
{
  const static QLatin1String PATTERN_MS("yyyy-MM-ddTHH:mm:ss.zzz");
  const static QLatin1String PATTERN("yyyy-MM-ddTHH:mm:ss");
  const static QString STR("%1%2:%3");

  int offset = dateTime.offsetFromUtc();
  return dateTime.toString(milliseconds ? PATTERN_MS : PATTERN) + STR.
         arg(offset >= 0 ? '+' : '-').
         arg(atools::absInt(offset / 3600), 2, 10, QChar('0')).
         arg(atools::absInt((offset / 60) % 60), 2, 10, QChar('0'));
}

QString currentIsoWithOffset(bool milliseconds)
{
  return convertToIsoWithOffset(QDateTime::currentDateTime(), milliseconds);
}

} // namespace atools
