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

#include "fs/weather/metarindex.h"

#include "geo/spatialindex.h"
#include "fs/weather/weathertypes.h"
#include "atools.h"

#include <QTimeZone>
#include <QRegularExpression>

namespace atools {
namespace fs {
namespace weather {

/* Internal struct for METAR data. Stored in the spatial index */
struct MetarData
{
  MetarData(const QString& identParam, const QString& metarParam, QDateTime timestampParam,
            const atools::geo::Pos& posParam)
    : ident(identParam), metar(metarParam), timestamp(timestampParam), pos(posParam)
  {
  }

  MetarData()
  {
  }

  QString ident, metar;
  QDateTime timestamp;

  bool isValid() const
  {
    return !ident.isEmpty();
  }

  atools::geo::Pos pos;

  const atools::geo::Pos& getPosition() const
  {
    return pos;
  }

};

// ====================================================================================================
MetarIndex::MetarIndex(MetarFormat formatParam, bool verboseLogging)
  : verbose(verboseLogging), format(formatParam)
{
  spatialIndex = new atools::geo::SpatialIndex<MetarData>;
}

MetarIndex::~MetarIndex()
{
  delete spatialIndex;
}

int MetarIndex::read(QTextStream& stream, const QString& fileOrUrl, bool merge)
{
  Q_ASSERT(format != UNKNOWN);
  Q_ASSERT(fetchAirportCoords);

  switch(format)
  {
    case atools::fs::weather::UNKNOWN:
      break;

    case atools::fs::weather::NOAA:
    case atools::fs::weather::XPLANE:
      return readNoaaXplane(stream, fileOrUrl, merge);

      break;

    case atools::fs::weather::FLAT:
      return readFlat(stream, fileOrUrl, merge);

      break;
  }
  return 0;
}

// 2017/07/30 18:45
// KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
//
// 2017/07/30 18:55
// KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
//
// 2017/07/30 18:47
// KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
int MetarIndex::readNoaaXplane(QTextStream& stream, const QString& fileOrUrl, bool merge)
{
  // Recognize METAR airport ident
  static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{2,5}$");

  // Recognize date part
  static const QRegularExpression DATE_REGEXP("^[\\d]{4}/[\\d]{2}/[\\d]{2}");

  if(!merge)
  {
    spatialIndex->clear();
    identIndexMap.clear();
  }

  QDateTime latest, oldest;
  QString latestIdent, oldestIdent;

  int lineNum = 1;
  QString line;
  QDateTime lastTimestamp, now = QDateTime::currentDateTimeUtc();
  int futureDates = 0, found = 0;

  while(!stream.atEnd())
  {
    line = stream.readLine().trimmed();

    if(line.isEmpty() || line.size() > 256)
    {
      lineNum++;
      continue;
    }

    if(format == XPLANE && (line.startsWith("MDEG ") || line.startsWith("DEG ")))
    {
      lineNum++;
      // Ignore X-Plane's special coordinate format
      continue;
    }

    if(line.size() >= 4)
    {

      if(DATE_REGEXP.match(line).hasMatch())
      {
        // Found line containing date like "2017/10/29 11:45"
        lastTimestamp = QDateTime::fromString(line, "yyyy/MM/dd hh:mm");
        lastTimestamp.setTimeZone(QTimeZone::utc());
        lineNum++;
        continue;
      }

      if(lastTimestamp > now)
      {
        // Ignore METARs with future UTC time
        futureDates++;
        lineNum++;
        continue;
      }

      QString ident = line.section(' ', 0, 0).simplified().toUpper();
      if(IDENT_REGEXP.match(ident).hasMatch())
      {
        // Found METAR line
        if(verbose)
        {
          if(!latest.isValid() || lastTimestamp > latest)
          {
            latest = lastTimestamp;
            latestIdent = ident;
          }
          if(!oldest.isValid() || lastTimestamp < oldest)
          {
            oldest = lastTimestamp;
            oldestIdent = ident;
          }
        }

        found++;
        updateOrInsert(line, ident, lastTimestamp);
      }
      else
        qWarning() << "Ident in METAR does not match in file/URL"
                   << fileOrUrl << "line num" << lineNum << "line" << line;
    }
    lineNum++;
  }

  updateIndex();

  if(verbose)
  {
    qDebug() << "index->size()" << spatialIndex->size();
    qDebug() << "metarMap.size()" << identIndexMap.size();
    qDebug() << "found " << found << "futureDates " << futureDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
  }

  qDebug() << Q_FUNC_INFO << fileOrUrl << spatialIndex->size();
  return found;
}

// KC99 100906Z AUTO 30022G42KT 10SM CLR M01/M04 A3035 RMK AO2
// LCEN 100920Z 16004KT 090V230 CAVOK 31/10 Q1010 NOSIG
// GMFO 100930Z 34004KT 240V010 9999 FEW030 26/00 Q1017 NOSIG
// KBLI 100922Z AUTO VRB04KT 10SM FEW017 FEW025 OVC037 13/11 A3015 RMK
// AGGH 100900Z 16003KT 9999 FEW016 FEW017CB SCT300 27/25 Q1010
// ANYN 100900Z 08005KT 9999 FEW020 27/23 Q1010
// AYMH 100800Z 16015KT 9999 SHRA BKN090 18/16 Q1018 RMK
int MetarIndex::readFlat(QTextStream& stream, const QString& fileOrUrl, bool merge)
{
  if(!merge)
  {
    spatialIndex->clear();
    identIndexMap.clear();
  }

  QDateTime latest, oldest;
  QString latestIdent, oldestIdent;

  int lineNum = 1;
  QString line;
  const QDateTime now = QDateTime::currentDateTimeUtc();
  int futureDates = 0, found = 0;

  while(!stream.atEnd())
  {
    line = stream.readLine().simplified().toUpper();

    if(line.isEmpty() || line.size() > 256)
    {
      lineNum++;
      continue;
    }

    if(line.size() >= 4)
    {
      QString ident = line.section(' ', 0, 0).simplified().toUpper();
      QString dateStr = line.section(' ', 1, 1).simplified().toUpper();

      if(ident.size() >= 3 && ident.size() <= 4 && dateStr.size() >= 5)
      {
        if(dateStr.endsWith('Z'))
          dateStr.chop(1);

        if(dateStr.size() == 5)
          dateStr.prepend('0');

        bool ok;
        int day = dateStr.mid(0, 2).toInt(&ok);
        if((!ok || day < 1 || day > 31) && verbose)
          qWarning() << Q_FUNC_INFO << "Cannot read day in METAR" << line;
        int hour = dateStr.mid(2, 2).toInt(&ok);
        if((!ok || hour < 0 || hour > 23) && verbose)
          qWarning() << Q_FUNC_INFO << "Cannot read hour in METAR" << line;
        int minute = dateStr.mid(4, 2).toInt(&ok);
        if((!ok || minute < 0 || minute > 60) && verbose)
          qWarning() << Q_FUNC_INFO << "Cannot read minute in METAR" << line;

        QDateTime metarDateTime = atools::correctDate(day, hour, minute);

        if(metarDateTime > now)
        {
          // Ignore METARs with future UTC time
          futureDates++;
          lineNum++;
          continue;
        }

        // Found METAR line
        if(verbose)
        {
          if(!latest.isValid() || metarDateTime > latest)
          {
            latest = metarDateTime;
            latestIdent = ident;
          }
          if(!oldest.isValid() || metarDateTime < oldest)
          {
            oldest = metarDateTime;
            oldestIdent = ident;
          }
        }

        found++;
        updateOrInsert(line, ident, metarDateTime);
      }
      else if(verbose)
        qWarning() << "Invalid METAR in file/URL" << fileOrUrl << "line num" << lineNum << "line" << line;
    }
    lineNum++;
  }

  updateIndex();

  if(verbose)
  {
    qDebug() << "index->size()" << spatialIndex->size();
    qDebug() << "metarMap.size()" << identIndexMap.size();
    qDebug() << "found " << found << "futureDates " << futureDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
  }

  qDebug() << Q_FUNC_INFO << fileOrUrl << spatialIndex->size();
  return found;
}

void MetarIndex::updateOrInsert(const QString& metar, const QString& ident, const QDateTime& lastTimestamp)
{
  int idx = identIndexMap.value(ident, -1);
  if(idx != -1)
  {
    // Already in list - get writeable reference to entry
    MetarData& md = (*spatialIndex)[idx];
    if(!md.timestamp.isValid() || md.timestamp < lastTimestamp)
    {
      // This one is newer - update
      md.metar = metar;
      md.timestamp = lastTimestamp;
    }
    // else leave as is
  }
  else
  {
    // Insert new record
    spatialIndex->append(MetarData(ident, metar, lastTimestamp, fetchAirportCoords(ident)));
    identIndexMap.insert(ident, spatialIndex->size() - 1);
  }
}

void MetarIndex::clear()
{
  spatialIndex->clear();
  identIndexMap.clear();
}

bool MetarIndex::isEmpty() const
{
  return spatialIndex->isEmpty();
}

int MetarIndex::size() const
{
  return spatialIndex->size();
}

atools::fs::weather::MetarResult MetarIndex::getMetar(const QString& station, const atools::geo::Pos& pos)
{
  atools::fs::weather::MetarResult result;
  result.init(station, pos);

  if(!station.isEmpty())
    result.metarForStation = metarData(station).metar;

  if(result.metarForStation.isEmpty() && pos.isValid())
  {
    MetarData data = spatialIndex->getNearest(pos);

    if(data.isValid())
    {
      // Found a METAR
      if(data.ident == station)
      {
        // Found exact match
        result.metarForStation = data.metar;

        if(station.isEmpty())
          result.requestIdent = data.ident;
      }
      else
      {
        // Found a station nearby
        result.metarForNearest = data.metar;

        if(station.isEmpty())
          result.requestIdent = data.ident;
      }
    }
  }

  return result;
}

void MetarIndex::updateIndex()
{
  Q_ASSERT(spatialIndex->size() == identIndexMap.size());
  spatialIndex->updateIndex();
}

MetarData MetarIndex::metarData(const QString& ident)
{
  int idx = identIndexMap.value(ident, -1);

  if(idx != -1)
    return spatialIndex->at(idx);

  return MetarData();
}

} // namespace weather
} // namespace fs
} // namespace atools
