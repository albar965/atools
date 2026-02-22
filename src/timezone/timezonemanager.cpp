/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "timezonemanager.h"

#include "atools.h"
#include "exception.h"
#include "geo/pos.h"
#include "timezone/library/zonedetect.h"

#include <QElapsedTimer>
#include <QFile>
#include <QTimeZone>

namespace atools {
namespace timezone {

class TimeZonePrivate
{
public:
  ZoneDetect *timezoneDb = nullptr;
  QByteArray library;

  static void onError(int errZD, int errNative)
  {
    qWarning() << Q_FUNC_INFO << QString(ZDGetErrorString(errZD)) << "0x" << Qt::hex << errNative;
    throw atools::Exception(TimeZoneManager::tr("Error in timezone: %1 / %2.").arg(QString(ZDGetErrorString(errZD))).arg(errNative));
  }

};

// ==========================================================
TimeZoneManager::TimeZoneManager(bool verboseParam)
  :verbose(verboseParam)
{
  p = new TimeZonePrivate;
}

TimeZoneManager::~TimeZoneManager()
{
  clear();
  delete p;
}

void TimeZoneManager::readFile(const QString& filename)
{
  ZDSetErrorHandler(TimeZonePrivate::onError);

  // Read file here and open library from memory since it cannot deal with UTF-8 paths
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    p->library = file.readAll();

    if(file.error() != QFile::NoError)
      throw atools::Exception(tr("Cannot read %1: %2").arg(filename).arg(file.errorString()));

    p->timezoneDb = ZDOpenDatabaseFromMemory(p->library.data(), p->library.size());
    if(!p->timezoneDb)
      throw atools::Exception(tr("Cannot read %1.").arg(filename));
  }

  qDebug() << Q_FUNC_INFO << "Opened" << filename << QString(ZDGetNotice(p->timezoneDb));
}

void TimeZoneManager::clear()
{
  ZDCloseDatabase(p->timezoneDb);
  p->timezoneDb = nullptr;
  p->library.clear();
  qDebug() << Q_FUNC_INFO << "Closed timezone database";
}

QTimeZone TimeZoneManager::getTimezone(const atools::geo::Pos& position) const
{
  return getTimezone(position.getLonX(), position.getLatY());
}

QTimeZone TimeZoneManager::getTimezone(float lonX, float latY) const
{
  QElapsedTimer timer;
  if(verbose)
    timer.start();
  char *string = ZDHelperSimpleLookupString(p->timezoneDb, latY, lonX);

  // float safezone = 0;
  // ZoneDetectResult *results = ZDLookup(cd, lat, lon, &safezone);
  // ZDLookupResultToString
  // ZDFreeResults

  if(verbose)
  {
    qDebug() << "Elapsed" << timer.nsecsElapsed() / 1000. << "us";
    qDebug() << "Simple string is" << QString(string);
  }

  QTimeZone zone(string);
  if(verbose)
    qDebug() << "Zone" << zone
             << "standard offset" << zone.standardTimeOffset(QDateTime::currentDateTime()) / 3600.f << "hours"
             << "UTC offset" << zone.offsetFromUtc(QDateTime::currentDateTime()) / 3600.f << "hours";

  ZDHelperSimpleLookupStringFree(string);
  return zone;
}

void TimeZoneManager::correctDateLocal(QDateTime& localDateTime, QDateTime& utcDateTime, int dayOfYearLocal, float secondsOfDayLocal,
                                       float secondsOfDayUtc, float lonX, float latY) const
{
  if(p->timezoneDb != nullptr)
  {
    const QDate localDate = QDate(QDate::currentDate().year(), 1, 1).addDays(dayOfYearLocal - 1);

    localDateTime = QDateTime(localDate,
                              QTime::fromMSecsSinceStartOfDay(atools::roundToInt(secondsOfDayLocal * 1000.f)),
                              QTimeZone(getTimezone(lonX, latY).standardTimeOffset(QDateTime(localDate, QTime(0, 0)))));
    utcDateTime = localDateTime.toOffsetFromUtc(0);
  }
  else
    // Use inaccurate method if database is not opened
    atools::correctDateLocal(localDateTime, utcDateTime, dayOfYearLocal, secondsOfDayLocal, secondsOfDayUtc, lonX);
}

} // namespace timezone
} // namespace atools
