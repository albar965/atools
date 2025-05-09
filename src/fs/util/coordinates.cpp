/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/util/coordinates.h"

#include "geo/pos.h"
#include "atools.h"
#include "exception.h"

#include <cmath>
#include <QRegularExpression>
#include <QLocale>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace util {

// N48194W123096
const static QString COORDS_FLIGHTPLAN_FORMAT_GFP("%1%2%3%4%5%6");
const static QRegularExpression LONG_FORMAT_REGEXP_GFP("^([NS])([0-9]{2})([0-9]{3})"
                                                       "([EW])([0-9]{3})([0-9]{3})$");

// 46N078W
const static QRegularExpression LONG_FORMAT_REGEXP_DEG("^([0-9]{2})([NS])"
                                                       "([0-9]{3})([EW])$");

// 4510N06810W
const static QString COORDS_FLIGHTPLAN_FORMAT_DEG_MIN("%1%2%3%4%5%6");
const static QRegularExpression LONG_FORMAT_REGEXP_DEG_MIN("^([0-9]{2})([0-9]{2})([NS])"
                                                           "([0-9]{3})([0-9]{2})([EW])$");

// 481200N0112842E
const static QString COORDS_FLIGHTPLAN_FORMAT_DEG_MIN_SEC("%1%2%3%4%5%6%7%8");
const static QRegularExpression LONG_FORMAT_REGEXP_DEG_MIN_SEC("^([0-9]{2})([0-9]{2})([0-9]{2})([NS])"
                                                               "([0-9]{3})([0-9]{2})([0-9]{2})([EW])$");

// Pattern allows trailing garbage
// 50:40:42 N 003:13:30 E
// 31:30:00 N 086:44:59 W
static QRegularExpression MATCH_COORD_OPENAIR_MIN_SEC("^([\\d]+):([\\d]+):([\\d\\.]+)\\s*([NS])\\s*"
                                                      "([\\d]+):([\\d]+):([\\d\\.]+)\\s*([EW])");

// Pattern allows trailing garbage
// 39:06.2 N 121:35.5 W
static QRegularExpression MATCH_COORD_OPENAIR_MIN("^([\\d]+):([\\d\\.]+)\\s*([NS])\\s*"
                                                  "([\\d]+):([\\d\\.]+)\\s*([EW])");

// ARINC full degreee waypoints
// 57N30 5730N 5730E 57E30
// 57W30 5730W 5730S 57S30
const static QRegularExpression LONG_FORMAT_REGEXP_ARINC("^([0-9]{2})"
                                                         "([0-9]{2})([NWES])$");
const static QRegularExpression LONG_FORMAT_REGEXP_ARINC2("^([0-9]{2})([NWES])"
                                                          "([0-9]{2})$");

// N6400 W07000 or N6400/W07000
const static QString COORDS_FLIGHTPLAN_FORMAT_PAIR("%1%2/%3%4");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR("^([NS])([0-9]{2})([0-9]{2})[ /]"
                                                        "([EW])([0-9]{3})([0-9]{2})$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR2("^([0-9]{2})([0-9]{2})([NS])[ /]"
                                                         "([0-9]{3})([0-9]{2})([EW])$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR_LAT("^([NS])([0-9]{2})([0-9]{2})$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR_LON("^([EW])([0-9]{3})([0-9]{2})$");

atools::geo::Pos degMinSecFormatFromCapture(const QStringList& captured);
atools::geo::Pos degMinFormatFromCapture(const QStringList& captured);

QString toGfpFormat(const atools::geo::Pos& pos)
{
  if(pos.isValid())
    return COORDS_FLIGHTPLAN_FORMAT_GFP.
           arg(pos.getLatY() > 0.f ? "N" : "S").
           arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLatYMin() + pos.getLatYSec() / 60.f) * 10.f, 3, 'f', 0, QChar('0')).
           arg(pos.getLonX() > 0.f ? "E" : "W").
           arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
           arg(std::abs(pos.getLonXMin() + pos.getLonXSec() / 60.f) * 10.f, 3, 'f', 0, QChar('0'));
  else
    return QString();
}

QString toDegMinFormat(const atools::geo::Pos& pos)
{
  if(pos.isValid())
    return COORDS_FLIGHTPLAN_FORMAT_DEG_MIN.
           arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLatYMin() + pos.getLatYSec() / 60.f), 2, 'f', 0, QChar('0')).
           arg(pos.getLatY() > 0.f ? "N" : "S").
           arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
           arg(std::abs(pos.getLonXMin() + pos.getLonXSec() / 60.f), 2, 'f', 0, QChar('0')).
           arg(pos.getLonX() > 0.f ? "E" : "W");
  else
    return QString();
}

QString toDegMinSecFormat(const atools::geo::Pos& pos)
{
  if(pos.isValid())
    return COORDS_FLIGHTPLAN_FORMAT_DEG_MIN_SEC.
           arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
           arg(atools::absInt(pos.getLatYMin()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLatYSec()), 2, 'f', 0, QChar('0')).
           arg(pos.getLatY() > 0.f ? "N" : "S").
           arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
           arg(atools::absInt(pos.getLonXMin()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLonXSec()), 2, 'f', 0, QChar('0')).
           arg(pos.getLonX() > 0.f ? "E" : "W");
  else
    return QString();
}

// Garmin format N48194W123096
atools::geo::Pos fromGfpFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_GFP.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 7)
    {
      bool latOk, lonOk, latMinOk, lonMinOk;
      QString ns = captured.at(1);
      int latYDeg = captured.at(2).toInt(&latOk);
      float latYMin = captured.at(3).toFloat(&latMinOk) / 10.f;
      float latYSec = (latYMin - std::floor(latYMin)) * 60.f;

      QString ew = captured.at(4);
      int lonXDeg = captured.at(5).toInt(&lonOk);
      float lonXMin = captured.at(6).toFloat(&lonMinOk) / 10.f;
      float lonXSec = (lonXMin - std::floor(lonXMin)) * 60.f;

      if(latOk && lonOk && latMinOk && lonMinOk &&
         -90 <= latYDeg && latYDeg <= 90 &&
         -180 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, static_cast<int>(lonXMin), lonXSec, ew == "W",
                                latYDeg, static_cast<int>(latYMin), latYSec, ns == "S");
    }
  }
  return atools::geo::EMPTY_POS;
}

// Degrees only 46N078W
atools::geo::Pos fromDegFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 5)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      QString ns = captured.at(2);

      int lonXDeg = captured.at(3).toInt(&lonOk);
      QString ew = captured.at(4);

      if(latOk && lonOk &&
         -90.f <= latYDeg && latYDeg <= 90.f &&
         -180.f <= lonXDeg && lonXDeg <= 180.f)
        return atools::geo::Pos(lonXDeg, 0, 0.f, ew == "W",
                                latYDeg, 0, 0.f, ns == "S");
    }
  }
  return atools::geo::EMPTY_POS;
}

// Degrees and minutes 4510N06810W
atools::geo::Pos fromDegMinFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG_MIN.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 7)
    {
      bool latOk, lonOk, latMinOk, lonMinOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int latYMin = captured.at(2).toInt(&latMinOk);
      QString ns = captured.at(3);

      int lonXDeg = captured.at(4).toInt(&lonOk);
      int lonXMin = captured.at(5).toInt(&lonMinOk);
      QString ew = captured.at(6);

      if(latOk && lonOk && latMinOk && lonMinOk &&
         -90 <= latYDeg && latYDeg <= 90 &&
         -180 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, lonXMin, 0.f, ew == "W",
                                latYDeg, latYMin, 0.f, ns == "S");
    }
  }

  return atools::geo::EMPTY_POS;
}

// Degrees, minutes and seconds 481200N0112842E
atools::geo::Pos fromDegMinSecFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG_MIN_SEC.match(str.simplified().toUpper());

  if(match.hasMatch())
    return degMinSecFormatFromCapture(match.capturedTexts());

  return atools::geo::EMPTY_POS;
}

// Degrees and minutes in pair N6400 W07000 or N6400/W07000
atools::geo::Pos fromDegMinPairFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_PAIR.match(str.simplified().toUpper());

  bool latOk = false, lonOk = false, latMinOk = false, lonMinOk = false;
  QString ns, ew;
  int latYDeg = 0, latYMin = 0, lonXDeg = 0, lonXMin = 0;

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();
    if(captured.size() == 7)
    {
      ns = captured.at(1);
      latYDeg = captured.at(2).toInt(&latOk);
      latYMin = captured.at(3).toInt(&latMinOk);
      ew = captured.at(4);
      lonXDeg = captured.at(5).toInt(&lonOk);
      lonXMin = captured.at(6).toInt(&lonMinOk);
    }
  }
  else
  {
    QRegularExpressionMatch match2 = LONG_FORMAT_REGEXP_PAIR2.match(str.simplified().toUpper());

    if(match2.hasMatch())
    {
      QStringList captured = match2.capturedTexts();
      if(captured.size() == 7)
      {
        latYDeg = captured.at(1).toInt(&latOk);
        latYMin = captured.at(2).toInt(&latMinOk);
        ns = captured.at(3);
        lonXDeg = captured.at(4).toInt(&lonOk);
        lonXMin = captured.at(5).toInt(&lonMinOk);
        ew = captured.at(6);
      }
    }
  }

  if(latOk && lonOk && latMinOk && lonMinOk &&
     -90 <= latYDeg && latYDeg <= 90 &&
     -180 <= lonXDeg && lonXDeg <= 180)
    return atools::geo::Pos(lonXDeg, lonXMin, 0.f, ew == "W",
                            latYDeg, latYMin, 0.f, ns == "S");
  else
    return atools::geo::EMPTY_POS;
}

// 57N30 5730N 5730E 57E30 57W30 5730W 5730S 57S30
atools::geo::Pos fromArincFormat(const QString& str)
{
  // 5730N 5730E 5730W 5730S
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_ARINC.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 4)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int lonXDeg = captured.at(2).toInt(&lonOk);

      if(latOk && lonOk)
      {
        QChar designator = atools::charAt(captured.at(3), 0);
        if(designator == 'N')
          lonXDeg = -lonXDeg;
        else if(designator == 'W')
        {
          lonXDeg = -lonXDeg;
          latYDeg = -latYDeg;
        }
        else if(designator == 'S')
          latYDeg = -latYDeg;

        Pos pos(static_cast<float>(lonXDeg), static_cast<float>(latYDeg));
        if(pos.isValidRange())
          return pos;
      }
    }
  }

  // 57N30 57E30 57W30 57S30 longitude + 100
  match = LONG_FORMAT_REGEXP_ARINC2.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 4)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int lonXDeg = captured.at(3).toInt(&lonOk) + 100;

      if(latOk && lonOk)
      {
        QChar designator = atools::charAt(captured.at(2), 0);
        if(designator == 'N')
          lonXDeg = -lonXDeg;
        else if(designator == 'W')
        {
          lonXDeg = -lonXDeg;
          latYDeg = -latYDeg;
        }
        else if(designator == 'S')
          latYDeg = -latYDeg;

        Pos pos(static_cast<float>(lonXDeg), static_cast<float>(latYDeg));
        if(pos.isValidRange())
          return pos;
      }
    }
  }

  return atools::geo::EMPTY_POS;
}

atools::geo::Pos fromAnyWaypointFormat(const QString& str)
{
  if(str.size() == 15)
    // Skyvector 481050N0113157E (N48°10.83' E11°31.96')
    return fromDegMinSecFormat(str);
  else if(str.size() == 13)
    // Garmin format N48194W123096
    return fromGfpFormat(str);
  else if(str.size() == 12)
    // "N6400 W07000" or "N6400/W07000"
    return fromDegMinPairFormat(str);
  else if(str.size() == 11)
    // Degrees and minutes 4510N06810W
    return fromDegMinFormat(str);
  else if(str.size() == 7)
    // Degrees only 46N078W or 34N150E
    return fromDegFormat(str);
  else if(str.size() == 5)
    // NAT type 5020N, 4122S 4424S or 73S00 72S15
    return fromArincFormat(str);

  return atools::geo::EMPTY_POS;
}

geo::Pos fromOpenAirFormat(const QString& coordStr)
{
  QRegularExpressionMatch match = MATCH_COORD_OPENAIR_MIN_SEC.match(coordStr.toUpper());
  if(match.hasMatch())
    return degMinSecFormatFromCapture(match.capturedTexts());
  else
  {
    match = MATCH_COORD_OPENAIR_MIN.match(coordStr);
    if(match.hasMatch())
      return degMinFormatFromCapture(match.capturedTexts());
  }

  return atools::geo::EMPTY_POS;
}

atools::geo::Pos degMinFormatFromCapture(const QStringList& captured)
{
  if(captured.size() == 7)
  {
    bool latOk, lonOk, latMinOk, lonMinOk;
    int latYDeg = captured.at(1).toInt(&latOk);
    float latYMin = captured.at(2).toFloat(&latMinOk);
    QString ns = captured.at(3);

    int lonXDeg = captured.at(4).toInt(&lonOk);
    float lonXMin = captured.at(5).toFloat(&lonMinOk);
    QString ew = captured.at(6);

    if(latOk && lonOk && latMinOk && lonMinOk &&
       -90 <= latYDeg && latYDeg <= 90 &&
       -180 <= lonXDeg && lonXDeg <= 180)
      return atools::geo::Pos((lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f),
                              (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f));
  }
  return atools::geo::EMPTY_POS;
}

atools::geo::Pos degMinSecFormatFromCapture(const QStringList& captured)
{
  if(captured.size() == 9)
  {
    bool latOk, lonOk, latMinOk, lonMinOk, latSecOk, lonSecOk;
    int latYDeg = captured.at(1).toInt(&latOk);
    int latYMin = captured.at(2).toInt(&latMinOk);
    float latYSec = captured.at(3).toFloat(&latSecOk);
    QString ns = captured.at(4);

    int lonXDeg = captured.at(5).toInt(&lonOk);
    int lonXMin = captured.at(6).toInt(&lonMinOk);
    float lonXSec = captured.at(7).toFloat(&lonSecOk);
    QString ew = captured.at(8);

    if(latOk && lonOk && latMinOk && lonMinOk && latSecOk && lonSecOk &&
       -90 <= latYDeg && latYDeg <= 90 &&
       -180 <= lonXDeg && lonXDeg <= 180)
      return atools::geo::Pos(lonXDeg, lonXMin, lonXSec, ew == "W",
                              latYDeg, latYMin, latYSec, ns == "S");
  }
  return atools::geo::EMPTY_POS;
}

QRegularExpressionMatch safeMatch(const QRegularExpression& regexp, const QString& str)
{
  if(!regexp.isValid())
    throw atools::Exception("Invalid regular expression: " + regexp.errorString() + " at " +
                            QString::number(regexp.patternErrorOffset()));
  else
    return regexp.match(str);
}

QString safeCaptured(const QRegularExpressionMatch& match, const QString& str)
{
  QString retval = match.captured(str);
  if(retval.isNull())
    throw atools::Exception("Match not found: " + str + " in " + match.regularExpression().pattern() +
                            ". Match groups: " + match.regularExpression().namedCaptureGroups().join(", "));
  return retval;
}

atools::geo::Pos degMinSecFromMatch(const QRegularExpressionMatch& match)
{
  QString ns = safeCaptured(match, "NS");
  int latYDeg = safeCaptured(match, "LATY_DEG").toInt();
  int latYMin = safeCaptured(match, "LATY_MIN").toInt();
  float latYSec = safeCaptured(match, "LATY_DEC_SEC").toFloat();

  QString ew = safeCaptured(match, "EW");
  int lonXDeg = safeCaptured(match, "LONX_DEG").toInt();
  int lonXMin = safeCaptured(match, "LONX_MIN").toInt();
  float lonXSec = safeCaptured(match, "LONX_DEC_SEC").toFloat();

  float latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (ns == "S" ? -1.f : 1.f);
  float lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (ew == "W" ? -1.f : 1.f);
  return Pos(lonX, latY);
}

atools::geo::Pos degMinDesignatorFromMatch(const QRegularExpressionMatch& match)
{
  QString ns = safeCaptured(match, "NS");
  int latYDeg = safeCaptured(match, "LATY_DEG").toInt();
  float latYMin = safeCaptured(match, "LATY_DEC_MIN").toFloat();

  QString ew = safeCaptured(match, "EW");
  int lonXDeg = safeCaptured(match, "LONX_DEG").toInt();
  float lonXMin = safeCaptured(match, "LONX_DEC_MIN").toFloat();

  float latY = (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f);
  float lonX = (lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f);
  return Pos(lonX, latY);
}

atools::geo::Pos degMinFromMatchGoogle(const QRegularExpressionMatch& match)
{
  bool latNegative = safeCaptured(match, "LATY_DEG_SIGN") == "-";
  int latYDeg = std::abs(safeCaptured(match, "LATY_DEG").toInt());
  float latYMin = std::abs(safeCaptured(match, "LATY_DEC_MIN").toFloat());

  bool lonNegative = safeCaptured(match, "LONX_DEG_SIGN") == "-";
  int lonXDeg = std::abs(safeCaptured(match, "LONX_DEG").toInt());
  float lonXMin = std::abs(safeCaptured(match, "LONX_DEC_MIN").toFloat());

  float latY = (latNegative ? -1.f : 1.f) * (latYDeg + latYMin / 60.f);
  float lonX = (lonNegative ? -1.f : 1.f) * (lonXDeg + lonXMin / 60.f);
  return Pos(lonX, latY);
}

atools::geo::Pos degFromMatchSigned(const QRegularExpressionMatch& match)
{
  bool ok;
  float latYDeg = safeCaptured(match, "LATY_DEC_DEG_SIGN").toFloat(&ok);
  if(!ok)
    return atools::geo::EMPTY_POS;

  float lonXDeg = safeCaptured(match, "LONX_DEC_DEG_SIGN").toFloat(&ok);
  if(!ok)
    return atools::geo::EMPTY_POS;

  return Pos(lonXDeg, latYDeg);
}

atools::geo::Pos degFromMatch(const QRegularExpressionMatch& match)
{
  float latYDeg = safeCaptured(match, "LATY_DEC_DEG").toFloat();
  QString ns = safeCaptured(match, "NS");

  float lonXDeg = safeCaptured(match, "LONX_DEC_DEG").toFloat();
  QString ew = safeCaptured(match, "EW");

  float latY = latYDeg * (ns == "S" ? -1.f : 1.f);
  float lonX = lonXDeg * (ew == "W" ? -1.f : 1.f);
  return Pos(lonX, latY);
}

geo::Pos fromAnyFormatInternal(const QString& coords, bool replaceDecimals, bool *hemisphere)
{
  if(hemisphere != nullptr)
    *hemisphere = true; // Default is N, E, W and S designators given

  if(coords.simplified().isEmpty())
    return atools::geo::EMPTY_POS;

  // Convert local dependent decimal point and comma to dot - i.e. allow comma. dot and
  // locale dependent separator in all languages
  QString coordStr(coords);

#ifdef DEBUG_INFORMATION_COORDS
  if(coordStr.count(',') == 1)
    coordStr.replace(',', ' ');
#endif

  // Replace variations for minute and degree signs like they are using in Wikipedia
  coordStr.replace(QChar(L'″'), '\"');
  coordStr.replace(QChar(L'′'), '\'');
  coordStr.replace(QChar(L'’'), '\'');
  coordStr.replace(QChar(L'`'), '\'');
  coordStr.replace(QChar(L'´'), '\'');

  // Remove various separators
  coordStr.replace('|', ' ');
  coordStr.replace(';', ' ');
  coordStr.replace('_', ' ');
  coordStr.replace(':', ' ');

  coordStr = coordStr.simplified().toUpper();

  if(replaceDecimals)
    coordStr = coordStr.replace(QLocale().decimalPoint(), QChar('.')).replace(',', '.');

  coordStr = coordStr.simplified();

  // North/south and east/west designator
  const static QLatin1String NS("(?<NS>[NS])");
  const static QLatin1String EW("(?<EW>[EW])");

  // Optional n-space
  const static QLatin1String SP("\\s*"); // Optional space
  const static QLatin1String SPREQ("\\s+"); // Required at least one space
  const static QLatin1String COMMA(",");

  // Degree, minute and seconds signs or space
  const static QString DEG("[ °\\*]");
  const static QLatin1String MIN("[ ']");
  const static QLatin1String SEC("[ \"]");

  // Degree, minute and seconds signs
  const static QString DEGEND("[°\\*]?");
  const static QLatin1String MINEND("[']?");
  const static QLatin1String SECEND("[\"]?");

  // Integer degree with named capture
  const static QLatin1String LONX_DEG("(?<LONX_DEG>[0-9]+)");
  const static QLatin1String LATY_DEG("(?<LATY_DEG>[0-9]+)");

  // Integer degree with named capture and sign
  const static QLatin1String LONX_DEG_SIGN("(?<LONX_DEG_SIGN>[+-]?)(?<LONX_DEG>[0-9]+)");
  const static QLatin1String LATY_DEG_SIGN("(?<LATY_DEG_SIGN>[+-]?)(?<LATY_DEG>[0-9]+)");

  // Decimal degree
  const static QLatin1String LONX_DEC_DEG("(?<LONX_DEC_DEG>[0-9\\.]+)");
  const static QLatin1String LATY_DEC_DEG("(?<LATY_DEC_DEG>[0-9\\.]+)");

  // Decimal degree with sign
  const static QLatin1String LONX_DEC_DEG_SIGN("(?<LONX_DEC_DEG_SIGN>[+-]?[0-9\\.]+)");
  const static QLatin1String LATY_DEC_DEG_SIGN("(?<LATY_DEC_DEG_SIGN>[+-]?[0-9\\.]+)");

  // Minutes
  const static QLatin1String LONX_MIN("(?<LONX_MIN>[0-9]+)");
  const static QLatin1String LATY_MIN("(?<LATY_MIN>[0-9]+)");

  // Decimal minutes
  const static QLatin1String LONX_DEC_MIN("(?<LONX_DEC_MIN>[0-9\\.]+)");
  const static QLatin1String LATY_DEC_MIN("(?<LATY_DEC_MIN>[0-9\\.]+)");

  // Decimal seconds
  const static QLatin1String LONX_DEC_SEC("(?<LONX_DEC_SEC>[0-9\\.]+)");
  const static QLatin1String LATY_DEC_SEC("(?<LATY_DEC_SEC>[0-9\\.]+)");

  // ================================================================================
  // Build regular expressions

  // N49° 26' 41.57" E9° 12' 5.49"
  static const QRegularExpression FORMAT_DEG_MIN_SEC_REGEXP(
    "^" + NS + SP + LATY_DEG + SP + DEG + SP + LATY_MIN + SP + MIN + SP + LATY_DEC_SEC + SP + SEC + SP +
    "" + EW + SP + LONX_DEG + SP + DEG + SP + LONX_MIN + SP + MIN + SP + LONX_DEC_SEC + SP + SECEND + "$");

  // 49° 26' 41,57" N 9° 12' 5,49" E
  static const QRegularExpression FORMAT_DEG_MIN_SEC_REGEXP2(
    "^" + LATY_DEG + SP + DEG + SP + LATY_MIN + SP + MIN + SP + LATY_DEC_SEC + SP + SECEND + SP + NS + SP +
    "" + LONX_DEG + SP + DEG + SP + LONX_MIN + SP + MIN + SP + LONX_DEC_SEC + SP + SECEND + SP + EW + "$");

  // 49° 26,69' N 9° 12,09' E
  static const QRegularExpression FORMAT_DEG_MIN_REGEXP2(
    "^" + LATY_DEG + SP + DEG + SP + LATY_DEC_MIN + SP + MIN + SP + NS + SP +
    "" + LONX_DEG + SP + DEG + SP + LONX_DEC_MIN + SP + MIN + SP + EW + "$");

  // N54* 16.82' W008* 35.95'
  static const QRegularExpression FORMAT_DEG_MIN_REGEXP(
    "^" + NS + SP + LATY_DEG + SP + DEG + SP + LATY_DEC_MIN + SP + MIN + SP +
    "" + EW + SP + LONX_DEG + SP + DEG + SP + LONX_DEC_MIN + SP + MINEND + SP + "$");

  // 49,4449° N 9,2015° E
  static const QRegularExpression FORMAT_DEG_REGEXP(
    "^" + LATY_DEC_DEG + SP + DEGEND + SP + NS + SP +
    "" + LONX_DEC_DEG + SP + DEGEND + SP + EW + "$");

  // N 49,4449° E 9,2015°
  static const QRegularExpression FORMAT_DEG_REGEXP2(
    "^" + NS + SP + LATY_DEC_DEG + SP + DEG + SP +
    "" + EW + SP + LONX_DEC_DEG + SP + DEGEND + "$");

  // Signed Lat lon or lon lat
  static const QRegularExpression FORMAT_NUMBER_SIGNED(
    "^" + LATY_DEC_DEG_SIGN + "[\\s\\|_/#;:]+" + LONX_DEC_DEG_SIGN + "$");

  // Google format -120 19.70, 46 42.88
  static const QRegularExpression FORMAT_NUMBER_GOOGLE(
    "^" + LATY_DEG_SIGN + SPREQ + LATY_DEC_MIN + SP + COMMA + SP + LONX_DEG_SIGN + SPREQ + LONX_DEC_MIN + "$");

  // ================================================================================
  // Decimal degree formats
  // 49,4449 -9,2015
  QRegularExpressionMatch match = safeMatch(FORMAT_NUMBER_SIGNED, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degFromMatchSigned(match);

    if(hemisphere != nullptr)
      *hemisphere = false; // caller probably has to swap lat/lon

    if(pos.isValid()) // Do not check range since lat/lon might be swapped
      return pos;
  }

  // 49,4449° N 9,2015° E
  match = safeMatch(FORMAT_DEG_REGEXP, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // N 49,4449° E 9,2015°
  match = safeMatch(FORMAT_DEG_REGEXP2, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // ================================================================================
  // Degree and decimal minute formats
  // N54* 16.82' W008* 35.95'
  match = safeMatch(FORMAT_DEG_MIN_REGEXP, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinDesignatorFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // 49° 26,69' N 9° 12,09' E
  match = safeMatch(FORMAT_DEG_MIN_REGEXP2, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinDesignatorFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // -120 19.70, 46 42.88
  match = safeMatch(FORMAT_NUMBER_GOOGLE, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinFromMatchGoogle(match);
    if(pos.isValidRange())
      return pos;
  }

  // ================================================================================
  // Degree, minute and second formats
  // N49° 26' 41.57" E9° 12' 5.49"
  match = safeMatch(FORMAT_DEG_MIN_SEC_REGEXP, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinSecFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // 49° 26' 41,57" N 9° 12' 5,49" E
  match = safeMatch(FORMAT_DEG_MIN_SEC_REGEXP2, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinSecFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  return fromAnyWaypointFormat(coordStr);
}

atools::geo::Pos fromAnyFormat(const QString& coords, bool *hemisphere)
{
  Pos pos = fromAnyFormatInternal(coords, true /* replaceDecimal */, hemisphere);

  if(!pos.isValid() && coords.count(", ") == 1)
    // Nothing found - modify format
    // Comma/space as number separator
    pos = fromAnyFormatInternal(coords, true /* replaceDecimal */, hemisphere);

  if(!pos.isValid())
  {
    if(coords.count(',') == 3)
      // Comma as decimal separator and comma as number separator
      pos = fromAnyFormatInternal(coords.section(',', 0, 1) + " " +
                                  coords.section(',', 2, 3), true /* replaceDecimal */, hemisphere);
    else if(coords.count(',') == 1 && coords.count('.') == 2)
      // Period as decimal separator and comma as number separator
      pos = fromAnyFormatInternal(coords.section(',', 0, 0) + " " +
                                  coords.section(',', 1, 1), true /* replaceDecimal */, hemisphere);
  }

  if(!pos.isValid())
    // Maybe Google earth format - use as is
    pos = fromAnyFormatInternal(coords, false /* replaceDecimal */, hemisphere);

  return pos;
}

void maybeSwapOrdinates(geo::Pos& pos, const QString& coords)
{
  if(!((coords.contains('N') || coords.contains('S')) && (coords.contains('E') || coords.contains('W'))))
    pos.swapLonXLatY();
}

} // namespace util
} // namespace fs
} // namespace atools
