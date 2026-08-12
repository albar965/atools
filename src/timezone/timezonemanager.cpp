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
  if(atools::checkFile(Q_FUNC_INFO, filename))
  {
    ZDSetErrorHandler(TimeZonePrivate::onError);

    // Read file here and open library from memory since it cannot deal with UTF-8 paths
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
      p->library = file.readAll();

      if(file.error() != QFile::NoError)
        throw atools::Exception(tr("Cannot read %1: %2").arg(filename, file.errorString()));

      if(p->library.isEmpty())
        throw atools::Exception(tr("Library from file %1 is empty").arg(filename));

      p->timezoneDb = ZDOpenDatabaseFromMemory(p->library.data(), p->library.size());

      if(!p->timezoneDb)
        throw atools::Exception(tr("Cannot read %1.").arg(filename));

      qDebug() << Q_FUNC_INFO << "Opened" << filename << QString(ZDGetNotice(p->timezoneDb));
      file.close();
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot open" << filename << "reason" << file.errorString();
  }
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
  if(p->timezoneDb == nullptr)
    return QTimeZone(); // Invalid

  QElapsedTimer timer;
  if(verbose)
    timer.start();

  char *lookupString = ZDHelperSimpleLookupString(p->timezoneDb, latY, lonX);
  QString timezoneStr(lookupString);
  ZDHelperSimpleLookupStringFree(lookupString);

  // float safezone = 0;
  // ZoneDetectResult *results = ZDLookup(p->timezoneDb, latY, lonX, &safezone);
  // const char *resultstr=ZDLookupResultToString(results->lookupResult);
  // ZDFreeResults(results);

  if(verbose)
  {
    qDebug() << "Elapsed" << timer.nsecsElapsed() / 1000. << "us";
    qDebug() << "Simple string is" << timezoneStr;
  }

  QTimeZone timezone(timezoneStr.toLatin1());

#ifdef Q_OS_MAC
  // Apply workaround to Qt bugs where QTimeZone fails to parse etc strings on macOS
  if(!timezone.isValid() && timezoneStr.startsWith(QStringLiteral("Etc/GMT"), Qt::CaseInsensitive))
  {
    timezoneStr.remove(0, 7);

    int offsetSeconds = 0;
    if(timezoneStr.contains(':'))
      // Hours : minutes
      offsetSeconds = timezoneStr.section(':', 0, 0).toInt() * 3600 + timezoneStr.section(':', 1, 1).toInt() * 60;
    else
      // Hours
      offsetSeconds = timezoneStr.toInt() * 3600;

    // Polarity is wrong
    offsetSeconds = -offsetSeconds;

    timezone = QTimeZone(offsetSeconds);
  }
#endif

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Zone" << timezone
             << "timezoneStr" << timezoneStr
             << "standard offset" << timezone.standardTimeOffset(QDateTime::currentDateTime()) / 3600.f << "hours"
             << "UTC offset" << timezone.offsetFromUtc(QDateTime::currentDateTime()) / 3600.f << "hours";

  return timezone;
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
