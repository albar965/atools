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

#include "fs/weather/metarindex.h"

#include "atools.h"
#include "fs/weather/metar.h"
#include "fs/weather/weathertypes.h"
#include "geo/calculations.h"
#include "geo/pos.h"
#include "geo/spatialindex.h"

#include <QTimeZone>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>

namespace atools {
namespace fs {
namespace weather {

/* Combines position and index in METAR arrays for spatial indexes */
class PosIndex
{
public:
  PosIndex()
  {
  }

  PosIndex(atools::geo::Pos posParam, int indexParam)
    : pos(posParam), index(indexParam)
  {
  }

  const atools::geo::Pos& getPosition() const
  {
    return pos;
  }

  atools::geo::Pos pos;
  int index; /* Index to "metarVector" or "metarInterpolatedVector" */
};

} // namespace weather
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::weather::PosIndex, Q_PRIMITIVE_TYPE);

namespace atools {
namespace fs {
namespace weather {

inline bool isDigit(QChar c)
{
  return c >= '0' && c <= '9';
}

inline bool isLetterOrDigit(QChar c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
}

// ====================================================================================================
MetarIndex::MetarIndex(MetarFormat formatParam, bool verboseLogging)
  : verbose(verboseLogging), format(formatParam)
{
  spatialIndex = new atools::geo::SpatialIndex<PosIndex>;
  spatialIndexInterpolated = new atools::geo::SpatialIndex<PosIndex>;
}

MetarIndex::~MetarIndex()
{
  ATOOLS_DELETE_LOG(spatialIndex);
  ATOOLS_DELETE_LOG(spatialIndexInterpolated);
}

int MetarIndex::read(const QString& filename, bool merge)
{
  int metarsRead = 0;
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    metarsRead = read(stream, filename, merge);
    file.close();
  }
  else
    qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();

  return metarsRead;
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

    case atools::fs::weather::FLAT:
      return readFlat(stream, fileOrUrl, merge);

    case atools::fs::weather::JSON:
      return readJson(stream, fileOrUrl, merge);
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
  if(!merge)
    clear();
  else
    clearCache();

  QDateTime latest, oldest;
  QString latestIdent, oldestIdent;

  int lineNum = 1;
  QString line;
  QDateTime lastTimestamp, now = QDateTime::currentDateTimeUtc();
  int futureDates = 0, invalidDates = 0, found = 0;

  while(!stream.atEnd())
  {
    line = stream.readLine().trimmed().toLatin1();

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
      // static const QRegularExpression DATE_REGEXP("^[\\d]{4}/[\\d]{2}/[\\d]{2}");
      if(line.size() > 10 &&
         // 2017/07/30 18:55 - detect only date part
         isDigit(line.at(0)) && isDigit(line.at(1)) && isDigit(line.at(2)) && isDigit(line.at(3)) &&
         line.at(4) == '/' && isDigit(line.at(5)) && isDigit(line.at(6)) &&
         line.at(7) == '/' && isDigit(line.at(8)) && isDigit(line.at(9)) &&
         line.at(10) == ' ')
      {
        // Found line containing date like "2017/10/29 11:45"
        lastTimestamp = QDateTime::fromString(line, "yyyy/MM/dd hh:mm");
        lastTimestamp.setTimeZone(QTimeZone::utc()); // Does not change date and time numbers
        lineNum++;
        continue;
      }

      if(!lastTimestamp.isValid())
      {
        // Ignore invalid dates
        invalidDates++;
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

      QString ident = line.section(' ', 0, 0);
      // Recognize METAR airport ident
      // static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{2,5}$");
      if(ident.size() >= 3 &&
         // KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
         isLetterOrDigit(line.at(0)) && isLetterOrDigit(line.at(1)) && isLetterOrDigit(line.at(2)))
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
        qWarning() << "Ident in METAR does not match in file/URL" << fileOrUrl << "line num" << lineNum << "line" << line;
    }
    lineNum++;
  }

  updateIndex();

  if(verbose)
  {
    qDebug() << "spatialIndex->size()" << spatialIndex->size();
    qDebug() << "spatialIndexInterpolated->size()" << spatialIndexInterpolated->size();
    qDebug() << "identIndexMap.size()" << identIndexMap.size();
    qDebug() << "metars.size()" << metarVector.size();
    qDebug() << "metarsInterpolated.size()" << metarInterpolatedVector.size();
    qDebug() << "found " << found << "futureDates " << futureDates << "invalidDates" << invalidDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
    qDebug() << Q_FUNC_INFO << fileOrUrl << spatialIndex->size();
  }

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
    clear();
  else
    clearCache();

  QDateTime latest, oldest;
  QString latestIdent, oldestIdent;

  int lineNum = 1;
  QString line;
  const QDateTime now = QDateTime::currentDateTimeUtc();
  int futureDates = 0, invalidDates = 0, found = 0;

  while(!stream.atEnd())
  {
    line = stream.readLine().simplified().toUpper().toLatin1();

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

        if(!metarDateTime.isValid())
        {
          // Ignore invalid dates
          invalidDates++;
          lineNum++;
          continue;
        }

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
    qDebug() << "spatialIndex->size()" << spatialIndex->size();
    qDebug() << "spatialIndexInterpolated->size()" << spatialIndexInterpolated->size();
    qDebug() << "identIndexMap.size()" << identIndexMap.size();
    qDebug() << "metars.size()" << metarVector.size();
    qDebug() << "metarsInterpolated.size()" << metarInterpolatedVector.size();
    qDebug() << "found " << found << "futureDates " << futureDates << "invalidDates" << invalidDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
    qDebug() << Q_FUNC_INFO << fileOrUrl << spatialIndex->size();
  }

  return found;
}

// .[
// .  {
// .    "airportIcao": "AGGH",
// .    "metar": "AGGH 091200Z 00000KT 9999 FEW016 SCT300 25/25 Q1010",
// .    "updatedAt": "2022-03-09T12:00:00.000Z"
// .  },
// .  {
// .    "airportIcao": "ANYN",
// .    "metar": "ANYN 091200Z 08006KT 9999 FEW020 28/25 Q1010",
// .    "updatedAt": "2022-03-09T12:00:00.000Z"
// .  },
// .  {
// .    "airportIcao": "AYMH",
// .    "metar": "AYMH 090800Z VRB04KT 9999 -SHRA SCT008 BKN030 -/- Q1019",
// .    "updatedAt": "2022-03-09T08:00:00.000Z"
// .  },
int MetarIndex::readJson(QTextStream& stream, const QString& fileOrUrl, bool merge)
{
  if(!merge)
    clear();
  else
    clearCache();

  QDateTime latest, oldest;
  QString latestIdent, oldestIdent;

  const QDateTime now = QDateTime::currentDateTimeUtc();
  int futureDates = 0, invalidDates = 0, found = 0;

  QJsonDocument doc = QJsonDocument::fromJson(stream.readAll().toUtf8());
  const QJsonArray arr = doc.array();
  for(const QJsonValue& airportValue : arr)
  {
    QJsonObject airportObj = airportValue.toObject();
    QString ident = airportObj.value("airportIcao").toString().toLatin1();
    QString metar = airportObj.value("metar").toString().toLatin1();
    QDateTime metarDateTime = QDateTime::fromString(airportObj.value("updatedAt").toString(), Qt::ISODateWithMs);

    if(!metarDateTime.isValid())
    {
      // Ignore invalid dates
      invalidDates++;
      continue;
    }

    if(metarDateTime > now)
    {
      // Ignore METARs with future UTC time
      futureDates++;
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
    updateOrInsert(metar, ident, metarDateTime);
  }

  updateIndex();

  if(verbose)
  {
    qDebug() << "spatialIndex->size()" << spatialIndex->size();
    qDebug() << "spatialIndexInterpolated->size()" << spatialIndexInterpolated->size();
    qDebug() << "identIndexMap.size()" << identIndexMap.size();
    qDebug() << "metars.size()" << metarVector.size();
    qDebug() << "metarsInterpolated.size()" << metarInterpolatedVector.size();
    qDebug() << "found " << found << "futureDates " << futureDates << "invalidDates" << invalidDates;
    qDebug() << "Latest" << latestIdent << latest;
    qDebug() << "Oldest" << oldestIdent << oldest;
    qDebug() << Q_FUNC_INFO << fileOrUrl << spatialIndex->size();
  }

  return found;
}

void MetarIndex::updateOrInsert(const QString& metarString, const QString& ident, const QDateTime& lastTimestamp)
{
  if(identIndexMap.contains(ident))
  {
    int idx = identIndexMap.value(ident, -1);
    if(idx != -1)
    {
      // Already in list - get writeable reference to entry
      Metar& metar = metarVector[idx];
      if(!metar.getTimestamp().isValid() || metar.getTimestamp() < lastTimestamp || metar.getStationMetar() != metarString)
      {
        // This one is newer - update
        metar.setMetarForStation(metarString);
        metar.setTimestamp(lastTimestamp);
        metar.parseAll(false /* useTimestamp */);
      }
    }
  }
  else
  {
    // Insert new record
    atools::geo::Pos pos = fetchAirportCoords(ident);

    Metar metar(ident, pos, lastTimestamp, metarString);
    metar.parseAll(false /* useTimestamp */);

    metarVector.append(metar);
    identIndexMap.insert(ident, metarVector.size() - 1);

    // Add only valid positions to spatial index
    if(pos.isValid())
      spatialIndex->append(PosIndex(pos, metarVector.size() - 1));
  }
}

void MetarIndex::clear()
{
  spatialIndex->clearIndex();
  identIndexMap.clear();
  metarVector.clear();
  clearCache();
}

void MetarIndex::clearCache()
{
  spatialIndexInterpolated->clearIndex();
  metarInterpolatedVector.clear();
}

bool MetarIndex::isEmpty() const
{
  return metarVector.isEmpty();
}

int MetarIndex::numStationMetars() const
{
  return metarVector.size();
}

const atools::fs::weather::Metar& MetarIndex::getMetar(const QString& station, atools::geo::Pos pos)
{
  const Metar& metar = fetchMetar(station);

  if(metar.hasAnyMetar())
    return metar;
  else
  {
    if(!pos.isValid() && !station.isEmpty())
      pos = fetchAirportCoords(station);

    if(pos.isValid())
    {
      int index = spatialIndexInterpolated->getNearestIndex(pos);

      if(index != -1)
      {
        // Found one interpolated near position - return if close enough ======================
        const Metar& metarInterpolated = metarInterpolatedVector.at(index);
        if(metarInterpolated.hasAnyMetar() && metarInterpolated.getRequestPos().distanceMeterTo(pos) < maxDistanceToleranceMeter)
          return metarInterpolated;
      }

      // Interpolate all nearest ======================
      QVector<PosIndex> posIndexes;
      spatialIndex->getNearest(posIndexes, pos, numInterpolation);

      // Collect positions ====================
      atools::fs::weather::MetarPtrVector metars;
      for(const PosIndex& posIndex : qAsConst(posIndexes))
        metars.append(&metarVector.at(posIndex.index));

      // Sort by distance to request point ====================
      std::sort(metars.begin(), metars.end(), [&pos](const Metar *t1, const Metar *t2) -> bool {
              return t1->getPosition().distanceMeterTo(pos) < t2->getPosition().distanceMeterTo(pos);
            });

      // Truncate above maximum distance, not parsed and having errors ===================
      float maxDistanceMeter = atools::geo::nmToMeter(maxDistanceInterpolationNm);
      metars.erase(std::remove_if(metars.begin(), metars.end(), [maxDistanceMeter, &pos](const Metar *m) -> bool {
              return m->getPosition().distanceMeterTo(pos) > maxDistanceMeter || !m->hasStationMetar() || m->getStation().hasErrors();
            }), metars.end());

      // Interpolate nearest metars =================================
      Metar metarInterpolatedNew = Metar(station, pos, metars);
      metarInterpolatedNew.parseAll(false /* useTimestamp */);

      // Clear full cache if too large
      if(metarInterpolatedVector.size() > maxInterpolatedCacheSize)
        clearCache();

      // Add to cache and return reference =======================
      metarInterpolatedVector.append(metarInterpolatedNew);
      spatialIndexInterpolated->append(PosIndex(pos, metarInterpolatedVector.size() - 1));
      return metarInterpolatedVector.constLast();
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot find METAR" << pos << station;
  }

  return Metar::EMPTY;
}

void MetarIndex::updateIndex()
{
  spatialIndex->updateIndex();
}

const Metar& MetarIndex::fetchMetar(const QString& ident) const
{
  if(!ident.isEmpty())
  {
    int idx = identIndexMap.value(ident, -1);

    if(idx != -1)
      return metarVector.at(idx);
  }

  return Metar::EMPTY;
}

} // namespace weather
} // namespace fs
} // namespace atools
