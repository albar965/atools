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

#ifndef ATOOLS_TIMEZONEMANAGER_H
#define ATOOLS_TIMEZONEMANAGER_H

#include <QCoreApplication>
#include <QTimeZone>

namespace atools {
namespace geo {
class Pos;
}
}
class QByteArray;
class QString;

namespace atools {
namespace timezone {

class TimeZonePrivate;

/*
 * Loads a timezone file using code from https://github.com/BertoldVdb/ZoneDetect.
 * Files are generated from https://github.com/evansiroky/timezone-boundary-builder and
 * https://naciscdn.org/naturalearth/10m/cultural.
 */
class TimeZoneManager
{
  Q_DECLARE_TR_FUNCTIONS(TimeZone)

public:
  explicit TimeZoneManager(bool verboseParam = false);
  ~TimeZoneManager();

  /* Reads time zone file into memory to prepare lookup. Throws exception in case of error. */
  void readFile(const QString& filename);

  /* Clears the read file from memory */
  void clear();

  /* Looks up a time zone id and locates the related QTimeZone object from Qt. */
  QTimeZone getTimezone(const atools::geo::Pos& position) const;
  QTimeZone getTimezone(float lonX, float latY) const;

  /* Determines timezone offset by seconds of day and creates localDateTime time from incomplete values based on current year.
   * Time can be converted to UTC which might also roll over the date.
   * Used to create a complete timedata from the cumbersome X-Plane time and date datarefs.
   * Uses an inaccurate lookup if database file was not loaded.
   * Does not use DST. */
  void correctDateLocal(QDateTime& localDateTime, QDateTime& utcDateTime, int dayOfYearLocal, float secondsOfDayLocal,
                        float secondsOfDayUtc, float lonX, float latY) const;

private:
  bool verbose = false;
  TimeZonePrivate *p;
};

} // namespace timezone
} // namespace atools

#endif // ATOOLS_TIMEZONEMANAGER_H
