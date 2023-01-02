/*****************************************************************************
* Copyright 2015-2022 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_ATOOLS_H
#define ATOOLS_ATOOLS_H

#include <cmath>

#include <QDebug>
#include <QLineF>
#include <QPointF>
#include <QTextCodec>

class QFile;
class QFileInfo;
class QFontMetrics;

namespace atools {

QString version();

QString gitRevision();

/* replace variables in a string like "${LANG}" */
QString replaceVar(QString str, const QString& name, const QVariant& value);
QString replaceVar(QString str, const QHash<QString, QVariant>& variableValues);

/* Program information which can be added to file headers:
 * Created by %1 Version %2 (revision %3) on %4*/
QString programFileInfo();
QString programFileInfoNoDate();

/* Return true if the file ends with an carriage return or line feed.
 *  An Exception is thrown if the file cannot be opened */
bool fileEndsWithEol(const QString& filepath);

/* Get the target of a symbolic link (all OS), a Windows shortcut (.lnk) or a Windows junction.
 * Returns a cleaned path with '/' as separators or an empty string if file is not link. */
QString linkTarget(const QFileInfo& path);

/* Get canonical path also considering symbolic links, Windows shortcuts or a Windows junctions.
 * Returns a cleaned path with '/' as separators. */
QString canonicalFilePath(const QFileInfo& path);
QString canonicalPath(const QFileInfo& path);

/* Return true if one of the elements in list is equal to str */
bool contains(const QString& name, const std::initializer_list<QString>& list);
bool contains(const QString& name, const std::initializer_list<const char *>& list);

template<typename TYPE>
bool contains(const TYPE& str, const std::initializer_list<TYPE>& list)
{
  for(const TYPE& val : list)
    if(val == str)
      return true;

  return false;
}

/* Returns true if the string contains one of the list */
bool strContains(const QString& name, const std::initializer_list<QString>& list);
bool strContains(const QString& name, const std::initializer_list<const char *>& list);
bool strContains(const QString& name, const std::initializer_list<char>& list);
bool strContains(const QString& name, const QStringList& list);

/* Concatenates all non empty strings in the list with the given separator */
QString strJoin(const QStringList& list, const QString& sep);

/* Concatenates all non empty strings in the list with the given separator and uses lastSep for separation.
 *  Example: strJoin({"A", "B", "C"}, ", ", " and ") => "A, B and C" */
QString strJoin(const QStringList& list, const QString& sep, const QString& lastSep, const QString& suffix = QString());
QString strJoin(const QString& prefix, const QStringList& list, const QString& sep, const QString& lastSep,
                const QString& suffix = QString());

/* true if str starts with any of the strings in the list  */
bool strStartsWith(const QStringList& list, const QString& str);

/* true if any string in the list starts str */
bool strAnyStartsWith(const QStringList& list, const QString& str);

template<typename TYPE1, typename TYPE2>
void convertList(QList<TYPE1>& dest, const QList<TYPE2>& src)
{
  for(TYPE2 type : src)
    dest.append(type);
}

template<typename TYPE1, typename TYPE2>
void convertVector(QVector<TYPE1>& dest, const QVector<TYPE2>& src)
{
  for(TYPE2 type : src)
    dest.append(type);
}

/* Functions for safe insert that allow and index < 0 and > size + 1 */
template<typename TYPE>
void insertInto(QList<TYPE>& list, int index, const TYPE& type)
{
  if(index < 0)
    list.prepend(type);
  else if(index > list.size())
    list.append(type);
  else
    list.insert(index, type);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename TYPE>
void insertInto(QVector<TYPE>& list, int index, const TYPE& type)
{
  if(index < 0)
    list.prepend(type);
  else if(index > list.size())
    list.append(type);
  else
    list.insert(index, type);
}

#endif

/* Read whole file into a string */
QString strFromFile(const QString& filename);

/* Write string into a file */
void strToFile(const QString& filename, const QString& text);

/* Unicode normalizes string and replaces special characters like ö->o.
 * Removes all diacritics.
 * Omits characters if they cannot be transformed to ASCII. */
QString normalizeStr(QString str);

/* Cuts text at the right and uses combined ellipsis character */
QString elideTextShort(const QString& str, int maxLength);
QStringList elideTextShort(QStringList list, int maxLength);

/* Cuts text at the left and uses combined ellipsis character */
QString elideTextShortLeft(const QString& str, int maxLength);

/* Cuts text in the center and uses combined ellipsis character */
QString elideTextShortMiddle(const QString& str, int maxLength);

#if defined(QT_WIDGETS_LIB)
/* Elide text for pixel size. Returns single "..." for less than three characters, a single "." if it fits and empty text if not. */
QString elidedText(const QFontMetrics& metrics, QString text, Qt::TextElideMode mode, int width);
QStringList elidedTexts(const QFontMetrics& metrics, const QStringList& texts, Qt::TextElideMode mode, int width);

#endif

/* Remove any non printable characters from string */
QString removeNonPrintable(const QString& str);

/* Remove all characters not being letter, number or punctuation */
QString removeNonAlphaNum(const QString& str);

/* Turns a string list into a string of blocked text.
 * E.g:
 * blockText({"A","B","C","D","E","F"}, 2, ",", "\n")
 * ->
 * "A,B,\nC,D,\nE,F"
 */
QString blockText(const QStringList& texts, int maxItemsPerLine, const QString& itemSeparator,
                  const QString& lineSeparator);

#if defined(QT_WIDGETS_LIB)
/* Fills maxWidth text lines into result using the separator. */
QStringList wrapText(const QStringList& texts, const QFontMetrics& metrics, int maxWidth, const QString& separator);

#endif

/* Cut linefeed separated text. Return maxLength lines where \n... is included
 * @param compressEmpty Remove empty lines before applying elide
 * @param ellipseLastLine Put ellipse on separate line */
QString elideTextLinesShort(QString str, int maxLines, int maxLength = 0, bool compressEmpty = false,
                            bool ellipseLastLine = true);

/* Concatenates all paths parts with the QDir::separator() and fetches names correcting the case */
QString buildPathNoCase(const QStringList& paths);

/* Simply concatenates all paths parts with the QDir::separator() */
QString buildPath(const QStringList& paths);

/* Read a part of the file and find out the text codec if it has a BOM.
 * File has to be open for reading.
 * Do not delete the returned pointer. */
QTextCodec *codecForFile(QIODevice& file, QTextCodec *defaultCodec = nullptr);

/* Get the first four lines of a file converted to lowercase to check type.
 *  Returns a list with always four strings.
 *  Empty lines are removed and line length is limited to 80 characters.
 *  All trimmed and converted to lower case. */
QStringList probeFile(const QString& file, int numLinesRead = 6);

/* Calculate the step size for an axis along a range for number of steps.
 * Steps will stick to the 1, 2, and 5 range */
float calculateSteps(float range, float numSteps);

/* different to std::fmod and std::remainder. Sign follows the divisor or be Euclidean. Remainder of x/y */
template<typename TYPE>
constexpr TYPE mod(TYPE x, TYPE y)
{
  return x - y *std::floor(x / y);
}

template<typename TYPE>
constexpr bool inRange(const QList<TYPE>& list, int index)
{
  return index >= 0 && index < list.size();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename TYPE>
constexpr bool inRange(const QVector<TYPE>& list, int index)
{
  return index >= 0 && index < list.size();
}

#endif

template<typename TYPE>
constexpr bool inRange(TYPE minValue, TYPE maxValue, TYPE value)
{
  return value >= minValue && value <= maxValue;
}

template<typename TYPE>
constexpr TYPE minmax(TYPE minValue, TYPE maxValue, TYPE value)
{
  return std::min(std::max(value, minValue), maxValue);
}

/* Char as string at position or empty string if out of bounds */
inline QString strAt(const QString& str, int index)
{
  return index >= 0 && index < str.size() ? str.at(index) : QString();
}

/* Returns 0 if without throwing an exception if index is not valid */
inline QChar charAt(const QString& str, int index)
{
  return index >= 0 && index < str.size() ? str.at(index) : QChar('\0');
}

/* Returns 0 if without throwing an exception if index is not valid */
inline char latin1CharAt(const QString& str, int index)
{
  return index >= 0 && index < str.size() ? str.at(index).toLatin1() : '\0';
}

template<typename TYPE>
const TYPE& at(const QList<TYPE>& list, int index, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size();
  return defaultType;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename TYPE>
const TYPE& at(const QVector<TYPE>& list, int index, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size();
  return defaultType;
}

#endif

template<typename TYPE>
const TYPE& at(const QList<TYPE>& list, int index, const QString& msg, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size() << "message" << msg;
  return defaultType;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename TYPE>
const TYPE& at(const QVector<TYPE>& list, int index, const QString& msg, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size() << "message" << msg;
  return defaultType;
}

#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/* Functions roll over to first position on overflow */
template<typename TYPE>
const TYPE& atRoll(const QVector<TYPE>& list, int index)
{
  return index < list.size() ? list.at(index) : list.constFirst();
}

#endif
template<typename TYPE>
const TYPE& atRoll(const QList<TYPE>& list, int index)
{
  return index < list.size() ? list.at(index) : list.constFirst();
}

/* Writes a warning message includiing the string list */
QString at(const QStringList& columns, int index, bool error);

/* Both write a warning message if number format is wrong */
int atInt(const QStringList& columns, int index, bool error);
float atFloat(const QStringList& columns, int index, bool error);

template<typename TYPE>
inline TYPE *firstOrNull(QList<TYPE>& list)
{
  return list.isEmpty() ? nullptr : &list.first();
}

template<typename TYPE>
inline const TYPE *constFirstOrNull(const QList<TYPE>& list)
{
  return list.isEmpty() ? nullptr : &list.constFirst();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename TYPE>
inline TYPE *firstOrNull(QVector<TYPE>& list)
{
  return list.isEmpty() ? nullptr : &list.first();
}

template<typename TYPE>
inline const TYPE *constFirstOrNull(const QVector<TYPE>& list)
{
  return list.isEmpty() ? nullptr : &list.constFirst();
}

#endif

static const int MAX_FILENAME_CHARS = 150;

/* Get a list of all invalid filename characters optionally escaped for HTML */
QString invalidFilenameCharacters(bool html);

/* Remove all special characters from the filename that can disturb any
 * filesystem and truncated the name in the middle */
QString cleanFilename(QString filename, int maxLength = MAX_FILENAME_CHARS);

constexpr int absInt(int value)
{
  return value > 0 ? value : -value;
}

constexpr long absLong(long value)
{
  return value > 0L ? value : -value;
}

constexpr long long absLongLong(long long value)
{
  return value > 0L ? value : -value;
}

/* Round to integer value */
template<typename TYPE>
constexpr int roundToInt(TYPE value)
{
  return static_cast<int>(std::round(value));
}

/* Linear interpolation
 * f(x) = f0 + ((f1 - f0) / (x1 - x0)) * (x - x0) */
template<typename TYPE>
constexpr TYPE interpolate(TYPE f0, TYPE f1, TYPE x0, TYPE x1, TYPE x)
{
  return f0 == f1 ? f0 : f0 + ((f1 - f0) / (x1 - x0)) * (x - x0);
}

/* Get coordinate where x crosses the line  */
constexpr QPointF interpolateForX(const QLineF& line, double x)
{
  return line.pointAt((x - line.x1()) / line.dx());
}

/* Get coordinate where y crosses the line */
constexpr QPointF interpolateForY(const QLineF& line, double y)
{
  return line.pointAt((y - line.y1()) / line.dy());
}

/* Extract the first latin1 character from string. Return null if string is empty */
inline char strToChar(const QString& str)
{
  return str.isEmpty() ? '\0' : str.at(0).toLatin1();
}

/* Extract the a latin1 character from string. Return null if string is empty */
inline char strToChar(const QString& str, int index)
{
  return str.isEmpty() ? '\0' : str.at(index).toLatin1();
}

/* Extract the first latin1 character from string. Return null if string is empty */
inline unsigned char strToUChar(const QString& str)
{
  return static_cast<unsigned char>(str.isEmpty() ? '\0' : str.at(0).toLatin1());
}

/* Extract the a latin1 character from string. Return null if string is empty */
inline unsigned char strToUChar(const QString& str, int index)
{
  return static_cast<unsigned char>(str.isEmpty() ? '\0' : str.at(index).toLatin1());
}

/* Return string build from char  */
inline QString charToStr(char c)
{
  return QString(QChar(c));
}

/* Round to precision (e.g. roundToPrecision(1111, 2) -> 1100) */
template<typename TYPE>
int roundToPrecision(TYPE value, int precision = 0)
{
  if(precision == 0)
    return static_cast<int>(round(value));
  else
  {
    int factor = static_cast<int>(std::pow(10., precision));
    return static_cast<int>(round(value / factor)) * factor;
  }
}

/* Round value to nearest multiple of round */
template<typename TYPE>
inline TYPE roundToNearest(TYPE value, TYPE round)
{
  return round > 0. ? std::round(value / round) * round : value;
}

/* To string with changing precision */
template<typename TYPE>
QString numberToString(TYPE value)
{
  int precision = 0;
  if(value < 10)
    precision = 2;
  else if(value < 100)
    precision = 1;
  return QString::number(value, 'f', precision);
}

/* Make first character upper case in given word */
QString capWord(QString str);

/* Capitalize all words in the string with exceptions that are either forced to upper or lower */
QString capString(const QString& str, const QSet<QString>& toUpper = {}, const QSet<QString>& toLower = {},
                  const QSet<QString>& ignore = {});

/* Returns a string containing value number of stars and maxValue - value number of dashes */
QString ratingString(int value, int maxValue);

/* Convert 24 hour and minute time string to time (500, 2314, 12:30) */
QTime timeFromHourMinStr(const QString& timeStr);

/* Rounds time down to last hour interval. E.g. 13:12 to 12:00 or 7:45 to 06:00  for a six hour interval */
QDateTime timeToLastHourInterval(QDateTime datetime, int intervalsPerDay);

/* Same as above but next interval */
QDateTime timeToNextHourInterval(QDateTime datetime, int intervalsPerDay);

/* Converts datetime to ISO8601 with timezone offset and milliseconds or not.
 * 2021-05-16T23:55:00.259+02:00 */
QString convertToIsoWithOffset(const QDateTime& datetime, bool milliseconds = true);

/* Current local datetime in ISO format as above */
QString currentIsoWithOffset(bool milliseconds = true);

/* Keep subtracting months for incomplete date and time until it is not in the future and the day matches
 * but not more than one year to avoid endless loops */
QDateTime correctDate(int day, int hour, int minute);

/* Determines timezone offset by seconds of day and creates local time from incomplete values based on current year.
 * Time can be converted to UTC which might also roll over the date. */
QDateTime correctDateLocal(int dayOfYear, int secondsOfDayLocal, int secondsOfDayUtc);
QDateTime correctDateLocalF(int dayOfYear, float secondsOfDayLocal, float secondsOfDayUtc);

template<typename TYPE>
constexpr int sign(TYPE t)
{
  if(static_cast<double>(t) > 0.)
    return 1;
  else if(static_cast<double>(t) < 0.)
    return -1;
  else
    return 0;
}

template<>
constexpr int sign<int>(int t)
{
  if(t > 0)
    return 1;
  else if(t < 0)
    return -1;
  else
    return 0;
}

template<typename TYPE>
constexpr bool almostEqual(TYPE f1, TYPE f2)
{
  return std::abs(f1 - f2) <= std::numeric_limits<TYPE>::epsilon();
}

template<typename TYPE>
constexpr bool almostEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return std::abs(f1 - f2) <= epsilon;
}

template<>
constexpr bool almostEqual<int>(int f1, int f2, int epsilon)
{
  return atools::absInt(f1 - f2) <= epsilon;
}

template<>
constexpr bool almostEqual<long>(long f1, long f2, long epsilon)
{
  return atools::absLong(f1 - f2) <= epsilon;
}

template<>
constexpr bool almostEqual<long long>(long long f1, long long f2, long long epsilon)
{
  return atools::absLongLong(f1 - f2) <= epsilon;
}

template<typename TYPE>
constexpr bool almostNotEqual(TYPE f1, TYPE f2)
{
  return !almostEqual<TYPE>(f1, f2);
}

template<typename TYPE>
constexpr bool almostNotEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return !almostEqual<TYPE>(f1, f2, epsilon);
}

template<>
constexpr bool almostNotEqual<int>(int f1, int f2, int epsilon)
{
  return !almostEqual<int>(f1, f2, epsilon);
}

template<>
constexpr bool almostNotEqual<long>(long f1, long f2, long epsilon)
{
  return !almostEqual<long>(f1, f2, epsilon);
}

template<>
constexpr bool almostNotEqual<long long>(long long f1, long long f2, long long epsilon)
{
  return !almostEqual<long long>(f1, f2, epsilon);
}

template<typename TYPE>
constexpr bool isValid(TYPE value)
{
  // max float (3.4028235 * 10^38) is used for all floating point numerical invalid values.
  // Divide by four to catch conversions of invalid value
  return value < std::numeric_limits<float>::max() / 4;
}

template<>
constexpr bool isValid<int>(int value)
{
  // 2 147 483 647
  return value < std::numeric_limits<int>::max() / 4;
}

/* Allocates array and fills with 0 */
template<typename TYPE>
inline TYPE *allocArray(int size)
{
  unsigned long num = static_cast<unsigned long>(size);
  TYPE *arr = new TYPE[num];
  memset(arr, 0, sizeof(arr[0]) * num);
  return arr;
}

/* Allocates array and fills with given type */
template<typename TYPE>
inline TYPE *allocArray(int size, const TYPE& fill)
{
  unsigned long num = static_cast<unsigned long>(size);
  TYPE *arr = new TYPE[num];
  for(int i = 0; i < size; i++)
    arr[i] = fill;
  return arr;
}

/* Frees array and sets pointer to nullptr */
template<typename TYPE>
inline void freeArray(TYPE *& arr)
{
  if(arr != nullptr)
    delete[] arr;
  arr = nullptr;
}

/* Functions to convert integer and float lists and vectors to string lists and back.
 * Can be used to store configuration lists.
 * Uses the C locale for number conversion.
 * ok has the same meaning as in QString::toInt().
 *
 * TYPE is converted to int */
template<typename TYPE>
QStringList numVectorToStrList(const QVector<TYPE>& vector)
{
  QStringList retval;
  for(int value : vector)
    retval.append(QString::number(static_cast<int>(value)));
  return retval;
}

template<typename TYPE>
QVector<TYPE> strListToNumVector(const QStringList& strings, bool *ok = nullptr)
{
  if(ok != nullptr)
    *ok = true;
  QVector<TYPE> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    int val = str.toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.append(static_cast<TYPE>(val));
  }
  return retval;
}

template<typename TYPE>
QStringList numSetToStrList(const QSet<TYPE>& set)
{
  QStringList retval;
  for(int value : set)
    retval.append(QString::number(static_cast<int>(value)));
  return retval;
}

template<typename TYPE>
QSet<TYPE> strListToNumSet(const QStringList& strings, bool *ok = nullptr)
{
  if(ok != nullptr)
    *ok = true;
  QSet<TYPE> retval;
  for(const QString& str : strings)
  {
    bool localOk;
    int val = str.toInt(&localOk);

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(static_cast<TYPE>(val));
  }
  return retval;
}

QStringList floatVectorToStrList(const QVector<float>& vector);

QVector<float> strListToFloatVector(const QStringList& strings, bool *ok = nullptr);
QStringList floatSetToStrList(const QSet<float>& set);

QSet<float> strListToFloatSet(const QStringList& strings, bool *ok = nullptr);

/* Functions to convert integer/float/string maps and hashes to string lists and back.
 * Can be used to store configuration lists.
 * ok has the same meaning as in QString::toInt().
 * string list contains consecutive key/value pairs. */
template<typename TYPE>
QStringList numStrHashToStrList(const QHash<TYPE, QString>& hash)
{
  QStringList retval;

  for(auto i = hash.constBegin(); i != hash.constEnd(); ++i)
  {
    retval.append(QString::number(static_cast<int>(i.key())));
    retval.append(i.value());
  }
  return retval;
}

template<typename TYPE>
QHash<TYPE, QString> strListToNumStrHash(const QStringList& strings, bool *ok = nullptr)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QHash<TYPE, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    TYPE val = static_cast<TYPE>(strings.at(i).toInt(&localOk));

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val, strings.at(i + 1));
  }
  return retval;
}

template<typename TYPE>
QStringList numStrMapToStrList(const QMap<TYPE, QString>& map)
{
  QStringList retval;

  for(auto i = map.constBegin(); i != map.constEnd(); ++i)
  {
    retval.append(QString::number(static_cast<int>(i.key())));
    retval.append(i.value());
  }
  return retval;
}

template<typename TYPE>
QMap<int, QString> strListToNumStrMap(const QStringList& strings, bool *ok = nullptr)
{
  Q_ASSERT((strings.size() % 2) == 0);

  if(ok != nullptr)
    *ok = true;

  QMap<TYPE, QString> retval;
  for(int i = 0; i < strings.size() - 1; i += 2)
  {
    bool localOk;
    TYPE val = static_cast<TYPE>(strings.at(i).toInt(&localOk));

    if(!localOk && ok != nullptr)
      *ok = false;

    retval.insert(val, strings.at(i + 1));
  }
  return retval;
}

QStringList floatStrHashToStrList(const QHash<float, QString>& hash);

QHash<float, QString> strListToFloatStrHash(const QStringList& strings, bool *ok = nullptr);
QStringList floatStrMapToStrList(const QMap<float, QString>& map);

QMap<float, QString> strListToFloatStrMap(const QStringList& strings, bool *ok = nullptr);

/* Get well known system folders from QStandardPaths. */
QString documentsDir();
QString downloadDir();
QString tempDir();
QString desktopDir();
QString homeDir();

/* Collect error messages for files or folders if they are readable, not empty and more.
 * Returns empty string if all is ok. */
QString checkDirMsg(const QFileInfo& dir, int maxLength = 80, bool warn = true);
QString checkFileMsg(const QFileInfo& file, int maxLength = 80, bool warn = true);
QString checkDirMsg(const QString& dir, int maxLength = 80, bool warn = true);
QString checkFileMsg(const QString& file, int maxLength = 80, bool warn = true);

/* Same as above but prints warnings into the log if flag is set and returns false if something is not ok.
 *  Use the Q_FUNC_INFO macro as parameter for funcInfo. */
bool checkFile(const QString& funcInfo, const QFileInfo& file, bool warn = true);
bool checkDir(const QString& funcInfo, const QFileInfo& dir, bool warn = true);
bool checkFile(const QString& funcInfo, const QString& file, bool warn = true);
bool checkDir(const QString& funcInfo, const QString& dir, bool warn = true);

/* Calculates a simple reproducible hash for all lines in the text file ignoring line endings.
 * Uses always the same seed and ignores empty lines. To be used for testing. */
uint textFileHash(const QString& filename);

/* Read a simple symmetrically encrypted string from the given file.
 * Can be used to obfuscate keys. Prints a warning and returns an empty string if the file does not exist. */
QString strFromCryptFile(const QString& filename, quint64 key);

} // namespace atools

#endif // ATOOLS_ATOOLS_H
