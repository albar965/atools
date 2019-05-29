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

#include "fs/weather/metarindex.h"

#include "geo/simplespatialindex.h"
#include "fs/weather/weathertypes.h"

#include <QFile>
#include <QTimeZone>
#include <QRegularExpression>

namespace atools {
namespace fs {
namespace weather {

MetarIndex::MetarIndex(int size, bool verboseLogging)
  : verbose(verboseLogging)
{
  index = new atools::geo::SimpleSpatialIndex<QString, MetarData>(size);
}

MetarIndex::~MetarIndex()
{
  delete index;
}

/* Get all ICAO codes that have a weather station */
QSet<QString> MetarIndex::getMetarAirportIdents() const
{
  return index->keys().toSet();
}

// 2017/07/30 18:45
// KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
//
// 2017/07/30 18:55
// KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
//
// 2017/07/30 18:47
// KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
bool MetarIndex::read(QTextStream& stream, const QString& fileOrUrl, bool merge)
{
  // Recognize METAR airport
  static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{2,5}$");

  // Recognize date part
  static const QRegularExpression DATE_REGEXP("^[\\d]{4}/[\\d]{2}/[\\d]{2}");

  if(!merge)
  {
    index->clear();
    metarMap.clear();
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

    if(line.isEmpty())
      continue;

    if(line.size() >= 4)
    {
      if(DATE_REGEXP.match(line).hasMatch())
      {
        // Found line containing date like "2017/10/29 11:45"
        lastTimestamp = QDateTime::fromString(line, "yyyy/MM/dd hh:mm");
        lastTimestamp.setTimeZone(QTimeZone::utc());
        continue;
      }

      if(lastTimestamp > now)
      {
        // Ignore METARs with future UTC time
        futureDates++;
        continue;
      }

      QString ident = line.section(' ', 0, 0);
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

        if(metarMap.contains(ident))
        {
          // Already in list - get writeable reference to entry
          MetarData& md = metarMap[ident];
          if(!md.timestamp.isValid() || md.timestamp < lastTimestamp)
          {
            // This one is newer - update
            md.metar = line;
            md.timestamp = lastTimestamp;
          }
          // else leave as is
        }
        else
          // Insert new record
          metarMap.insert(ident, {ident, line, lastTimestamp});
      }
      else
      {
        if(fileOrUrl.isEmpty())
          qWarning() << "Metar does not match in line num" << lineNum << "line" << line;
        else
          qWarning() << "Metar does not match in file" << fileOrUrl << "line num" << lineNum << "line" << line;
      }
    }
    lineNum++;
  }

  updateIndex();

  if(verbose)
  {
    qDebug() << "index->size()" << index->size();
    qDebug() << "metarMap.size()" << metarMap.size();
    qDebug() << "found " << found << "futureDates " << futureDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
  }

  qDebug() << Q_FUNC_INFO << fileOrUrl << index->size();

  return true;
}

void MetarIndex::clear()
{
  index->clear();
  metarMap.clear();
}

bool MetarIndex::isEmpty() const
{
  return index->isEmpty();
}

atools::fs::weather::MetarResult MetarIndex::getMetar(const QString& station, const atools::geo::Pos& pos)
{
  atools::fs::weather::MetarResult result;
  result.init(station, pos);

  MetarData data;
  QString foundKey = index->getTypeOrNearest(data, station, pos);
  if(!foundKey.isEmpty())
  {
    // Found a METAR
    if(foundKey == station)
      // Found exact match
      result.metarForStation = data.metar;
    else
      // Found a station nearby
      result.metarForNearest = data.metar;
  }

  return result;
}

void MetarIndex::updateIndex()
{
  index->clear();

  for(const QString& ident : metarMap.keys())
  {
    // Starts with an airport ident - add if position is valid
    atools::geo::Pos pos = fetchAirportCoords(ident);
    if(pos.isValid())
      index->insert(ident, metarMap.value(ident), pos);
  }
}

QString MetarIndex::getMetar(const QString& ident)
{
  return index->value(ident).metar;
}

} // namespace weather
} // namespace fs
} // namespace atools
