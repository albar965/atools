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

#include "fs/util/coordinates.h"

#include "geo/pos.h"
#include "atools.h"
#include "exception.h"

#include <cmath>
#include <QRegularExpression>

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

// 5020N
const static QRegularExpression LONG_FORMAT_REGEXP_NAT("^([0-9]{2})"
                                                       "([0-9]{2})N$");

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

// N48194W123096
// Examples:
// :F:N44124W122451 (User Waypoint: N44° 12.4' W122° 45.1'
// :F:N14544W017479 (User Waypoint: N14° 54.4' W17° 47.9'
// :F:S31240E136502 (User Waypoint: S31° 24.0' E136° 50.2'
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

// 4510N06810W
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

// Skyvector 481050N0113157E
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

// NAT type 5020N
// first two figures are the latitude north and the second two figures are the longitude west
atools::geo::Pos fromNatFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_NAT.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 3)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int lonXDeg = captured.at(2).toInt(&lonOk);

      if(latOk && lonOk &&
         0 <= latYDeg && latYDeg <= 90 &&
         0 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, 0, 0.f, true, latYDeg, 0, 0.f, false);
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
  else if(str.size() == 7)
    // Degrees only 46N078W
    return fromDegFormat(str);
  else if(str.size() == 11)
    // Degrees and minutes 4510N06810W
    return fromDegMinFormat(str);
  else if(str.size() == 5)
    // NAT type 5020N
    return fromNatFormat(str);

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
    throw  atools::Exception(
            "Invalid regular expression: " + regexp.errorString() + " at " +
            QString::number(regexp.patternErrorOffset()));
  else
    return regexp.match(str);
}

QString safeCaptured(const QRegularExpressionMatch& match, const QString& str)
{
  QString retval = match.captured(str);
  if(retval.isNull())
    throw  atools::Exception("Match not found: " + str + " in " + match.regularExpression().pattern() +
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

atools::geo::Pos degMinFromMatch(const QRegularExpressionMatch& match)
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

geo::Pos fromAnyFormat(const QString& coords)
{
  if(coords.simplified().isEmpty())
    return atools::geo::EMPTY_POS;

  // Convert local dependent decimal point and comma to dot - i.e. allow comma. dot and
  // locale dependent separator in all languages
  QString coordStr(coords.simplified().toUpper().replace(QLocale().decimalPoint(), ".").replace(",", "."));

  // North/south and east/west designator
  const static QLatin1Literal NS("(?<NS>[NS])");
  const static QLatin1Literal EW("(?<EW>[EW])");

  // Optional n-space
  const static QLatin1Literal SP("\\s*");

  // Degree, minute and seconds signs or space
  const static QLatin1Literal DEG("[ °\\*]");
  const static QLatin1Literal MIN("[ ']");
  const static QLatin1Literal SEC("[ \"]");

  // Degree, minute and seconds signs
  const static QLatin1Literal DEGEND("[°\\*]?");
  const static QLatin1Literal MINEND("[']?");
  const static QLatin1Literal SECEND("[\"]?");

  // Integer degree with named capture
  const static QLatin1Literal LONX_DEG("(?<LONX_DEG>[0-9]+)");
  const static QLatin1Literal LATY_DEG("(?<LATY_DEG>[0-9]+)");

  // Decimal degree
  const static QLatin1Literal LONX_DEC_DEG("(?<LONX_DEC_DEG>[0-9\\.]+)");
  const static QLatin1Literal LATY_DEC_DEG("(?<LATY_DEC_DEG>[0-9\\.]+)");

  // Minutes
  const static QLatin1Literal LONX_MIN("(?<LONX_MIN>[0-9]+)");
  const static QLatin1Literal LATY_MIN("(?<LATY_MIN>[0-9]+)");

  // Decimal minutes
  const static QLatin1Literal LONX_DEC_MIN("(?<LONX_DEC_MIN>[0-9\\.]+)");
  const static QLatin1Literal LATY_DEC_MIN("(?<LATY_DEC_MIN>[0-9\\.]+)");

  // Decimal seconds
  const static QLatin1Literal LONX_DEC_SEC("(?<LONX_DEC_SEC>[0-9\\.]+)");
  const static QLatin1Literal LATY_DEC_SEC("(?<LATY_DEC_SEC>[0-9\\.]+)");

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

  // ================================================================================
  // Decimal degree formats
  // 49,4449° N 9,2015° E
  QRegularExpressionMatch match = safeMatch(FORMAT_DEG_REGEXP, coordStr);
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
    Pos pos = degMinFromMatch(match);
    if(pos.isValidRange())
      return pos;
  }

  // 49° 26,69' N 9° 12,09' E
  match = safeMatch(FORMAT_DEG_MIN_REGEXP2, coordStr);
  if(match.hasMatch())
  {
    Pos pos = degMinFromMatch(match);
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

} // namespace util
} // namespace fs
} // namespace atools
