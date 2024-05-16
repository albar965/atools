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

#include "metar.h"
#include "fs/weather/metarparser.h"
#include "geo/calculations.h"

#include <QRegularExpression>

namespace atools {
namespace fs {
namespace weather {

const Metar Metar::EMPTY;

Metar::Metar(const QString& requestIdentParam, const geo::Pos& posParam, const MetarPtrVector& metars)
  :  requestIdent(requestIdentParam), requestPos(posParam), timestamp(QDateTime::currentDateTimeUtc())
{
  if(!metars.isEmpty())
  {
    // Set nearest always
    nearest = metars.constFirst()->getStation();

    // Add interpolated only if far away
    if(metars.constFirst()->getRequestPos().distanceMeterTo(posParam) > minInterpolationDistMeter)
    {
      // Collect distances and METARs
      QVector<float> distancesMeter;
      MetarParserVector parsedMetars;
      for(const Metar *metar : metars)
      {
        distancesMeter.append(metar->getRequestPos().distanceMeterTo(posParam));
        parsedMetars.append(metar->getStation());

        // Remember idents and positions
        interpolatedIdents.append(metar->getRequestIdent());
        interpolatedPositions.append(metar->getRequestPos());
      }

      interpolated = MetarParser::merge(parsedMetars, distancesMeter);
    }

    // Set or keep empty
    setRequestIdent(requestIdentParam);
  }
}

void Metar::parseAll(bool useTimestamp)
{
  station.parse();
  interpolated.parse();
  nearest.parse();

  if(useTimestamp)
  {
    if(station.isParsed())
      timestamp = station.getTimestamp();
    else if(nearest.isParsed())
      timestamp = nearest.getTimestamp();
    else if(interpolated.isParsed())
      timestamp = interpolated.getTimestamp();
  }
}

void Metar::clearAll()
{
  station.resetParsed();
  station.setMetar(QString());

  interpolated.resetParsed();
  interpolated.setMetar(QString());

  nearest.resetParsed();
  nearest.setMetar(QString());
}

void Metar::cleanFsxP3dAll()
{
  if(station.isFsxP3dFormat())
    station.setMetar(cleanMetar(station.getMetarString()));
  if(interpolated.isFsxP3dFormat())
    interpolated.setMetar(cleanMetar(interpolated.getMetarString()));
  if(nearest.isFsxP3dFormat())
    nearest.setMetar(cleanMetar(nearest.getMetarString()));
}

const MetarParser& Metar::getMetarParser(MetarType type) const
{
  switch(type)
  {
    case BEST:
      if(station.isParsed())
        return station;
      else if(interpolated.isParsed())
        return interpolated;
      else if(nearest.isParsed())
        return nearest;
      else
        return MetarParser::EMPTY;

    case STATION:
      return station;

    case INTERPOLATED:
      return interpolated;

    case NEAREST:
      return nearest;

    case NONE:
      return MetarParser::EMPTY;
  }

  return MetarParser::EMPTY;
}

// K53S&A1 000000Z 24705G06KT&D975NG 13520KT&A1528NG 129V141 9999 2ST025&ST001FNVN002N
// 1CI312&CI001FNVN002N 13/12 07/05&A1528 Q1009 @@@ 50 7 135 20 |
QString Metar::cleanMetar(const QString& metar)
{
  const static QRegularExpression CLOUD_EXTENSION("(CI|CS|CC|AS|AC|SC|NS|ST|CU|CB)...[FRA][NOLMHS]([VLMHD])([NRFHS])(\\d\\d\\d)[NTLMS]");
  const static QVector<QString> CLOUD_DENSITIES({"CLR", "FEW", "FEW", "SCT", "SCT", "BKN", "BKN", "BKN", "OVC"});
  const static QRegularExpression WIND("^(\\d{3}|VRB)\\d{1,3}(G\\d{2,3})?(KT|KMH|MPS)$");
  const static QRegularExpression VARIABLE_WIND("^\\d{3}V\\d{3}$");
  const static QRegularExpression LONG_VISIBILITY("^(\\d{3})(SM|KM)$");
  const static QRegularExpression VISIBILITY_KM("^(\\d{1,2})KM$");
  const static QRegularExpression TEMPERATURE("^([-M]?)(\\d{1,2})/([-M]?)(\\d{1,2})$");
  const static QRegularExpression CLOUD("^([0-8])(CI|CS|CC|AS|AC|SC|NS|ST|CU|CB)([0-9]{3})$");

  // CI Cirrus
  // CS Cirro-stratus (maps to CI)
  // CC Cirro-cumulus (maps to CI)
  // AS Alto-stratus (maps to ST)
  // AC Alto-cumulus (maps to CU)
  // SC Strato-cumulus (maps to CU)
  // NS Nimbo-stratus (maps to ST)
  // ST Stratus
  // CU Cumulus
  // CB Cumulo-nimbus
  // Note that not all of these cloud types are supported, so a number are
  // mapped to those which are. This does mean that a write followed by a read of
  // Metar data might not give identical strings.
  // ESP extension:
  // &TT000FTPQBBBI
  // where:
  // TT - Cloud type, one of: the list above (for example, CI or CB).
  // If this entry is different from the NNN entry above, this entry will take priority.
  // 000 - Unused.
  // F - Top of cloud, one of: F (flat), R (round), A (anvil)
  // T - Turbulence, one of: N - None (default), O - Light, L - Light, M - Moderate, H - Heavy, S - Severe
  // P - Precipitation, one of: V (very light), L (light), M (moderate), H (heavy) D (dense)
  // Q - Type of precipitation, one of: N (none), R (rain), F (freezing rain), H (hail), S (snow)
  // BBB - Coded base height, the precipitation ends at this height, set to 0 for it to land on the ground
  // I - icing rate, one of: N (none), T (trace), L (light), M (moderate), S (severe)

  QString cleanMetar;

  int numWind = 0, numVar = 0, numTmp = 0;
  // FSX gives the precipidation indication only in the cloud extension
  const QStringList met = metar.section("@@@", 0, 0).simplified().split(" ");
  QString precipitation;
  for(const QString& str : met)
  {
    QString extension = str.section("&", 1, 1).simplified();

    QRegularExpressionMatch extensionCloudMatch = CLOUD_EXTENSION.match(extension);
    if(extensionCloudMatch.hasMatch())
    {
      // V (very light), L (light), M (moderate), H (heavy) D (dense)
      QString intensity = extensionCloudMatch.captured(2);
      // Type of precipitation, one of: N (none), R (rain), F (freezing rain), H (hail), S (snow)
      QString type = extensionCloudMatch.captured(3);
      int altitude = extensionCloudMatch.captured(4).toInt();
      if(altitude == 0)
      {
        // If altitude is 0 precipitation extends to the ground
        if(intensity == "V" || intensity == "L")
          precipitation += "-";
        // else if(intensity == "M")
        else if(intensity == "H" || intensity == "D")
          precipitation += "+";

        if(type == "N")
          precipitation.clear();
        else if(type == "R")
          precipitation += "RA";
        else if(type == "F")
          precipitation += "RAFZ";
        else if(type == "H")
          precipitation += "FR";
        else if(type == "S")
          precipitation += "SN";
        break;
      }
    }
  }

  bool precipitationAdded = false;
  QStringList retval;
  for(const QString& str : met)
  {
    QString cleanStr = str.section("&", 0, 0).simplified();

    if(cleanStr == "000000Z" && timestamp.isValid())
      // Fix empty timestamp
      cleanStr = QLocale(QLocale::C).toString(timestamp, "ddHHmm") + "Z";
    else if(cleanStr == "????")
      // Replace pattern from interpolated with real station name
      cleanStr = requestIdent;
    else
    {
      bool isWind = WIND.match(cleanStr).hasMatch();
      bool isVar = VARIABLE_WIND.match(cleanStr).hasMatch();

      if(isWind)
        numWind++;
      if(isVar)
        numVar++;

      if(isWind && numWind > 1)
        // Skip all duplicate wind indications
        continue;
      if(isVar && numVar > 1)
        // Skip all duplicate visibility indications
        continue;

      QRegularExpressionMatch visMatch = LONG_VISIBILITY.match(cleanStr);
      if(visMatch.hasMatch())
        // Fix too long three digit visibility
        cleanStr = "99" + visMatch.captured(2);

      QRegularExpressionMatch visMatchSm = VISIBILITY_KM.match(cleanStr);
      if(visMatchSm.hasMatch())
        // Convert visibility from km to nm
        cleanStr =
          QString::number(atools::roundToInt(atools::geo::meterToMi(visMatchSm.captured(1).toFloat() * 1000.f))) + "SM";

      QRegularExpressionMatch tmpMatch = TEMPERATURE.match(cleanStr);
      if(tmpMatch.hasMatch())
      {
        // Fix temperature - 1-2 digits to 2 digits and "-" to "M"
        numTmp++;

        if(numTmp > 1)
          // Skip duplicate temperature indications
          continue;

        cleanStr = QString("%1%2/%3%4").
                   arg(tmpMatch.captured(1).replace("-", "M")).
                   arg(tmpMatch.captured(2).toInt(), 2, 10, QChar('0')).
                   arg(tmpMatch.captured(3).replace("-", "M")).
                   arg(tmpMatch.captured(4).toInt(), 2, 10, QChar('0'));
      }

      QRegularExpressionMatch cldMatch = CLOUD.match(cleanStr);
      if(cldMatch.hasMatch())
      {
        if(!precipitationAdded)
        {
          // Add the precipitation as extracted from the cloud extension
          retval.append(precipitation);
          precipitationAdded = true;
        }

        // Cloud density from oktas to text
        cleanStr = CLOUD_DENSITIES.at(cldMatch.captured(1).toInt());

        // Skip cloud type and add altitude
        cleanStr += cldMatch.captured(3);
      }
    }
    retval.append(cleanStr);
  }

  cleanMetar = retval.join(" ").simplified().toUpper();

  return cleanMetar;
}

QDebug operator<<(QDebug out, const Metar& metar)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "Metar[requestIdent " << metar.requestIdent
                          << ", requestPos " << metar.requestPos
                          << ", timestamp " << metar.timestamp
                          << endl
                          << "station \"" << metar.station.getMetarString() << "\" parsed " << metar.station.isParsed()
                          << " " << metar.getStation().getFlightRulesString() << endl
                          << "nearest \"" << metar.nearest.getMetarString() << "\" parsed " << metar.nearest.isParsed()
                          << " " << metar.getNearest().getFlightRulesString() << endl
                          << "interpolated \"" << metar.interpolated.getMetarString() << "\" parsed " << metar.interpolated.isParsed()
                          << " " << metar.getNearest().getFlightRulesString() << endl
                          << "]";
  return out;
}

} // namespace weather
} // namespace fs
} // namespace atools
