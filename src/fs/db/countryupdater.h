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

#ifndef ATOOLS_COUNTRYREPAIR_H
#define ATOOLS_COUNTRYREPAIR_H

#include <QHash>

namespace atools {

namespace geo {
class Pos;
}

namespace timezone {
class TimeZoneManager;
}

namespace sql {
class SqlDatabase;
}
namespace fs {
namespace db {

/*
 * Corrects airport country names bei either using a list of known misspellings, converting codes
 * which were added instead of names or using time zones to detect name.
 */
class CountryUpdater
{
public:
  CountryUpdater(atools::sql::SqlDatabase& sqlDb, const QString& timezoneFile, bool verboseParam);
  ~CountryUpdater();

  /* Fix broken country name. Needs coordinates for time zone lookup. */
  QString updateAirportCountry(const QString& country, const atools::geo::Pos& pos);

private:
  atools::sql::SqlDatabase& db;
  atools::timezone::TimeZoneManager *timezone = nullptr;
  const static QHash<QString, QString> countries, /* Wrong country names with replacements */
                                       country3To2; /* convert ISO 3166-1 alpha-3 to alpha-2 for QLocale::codeToTerritory() */
  bool verbose;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_COUNTRYREPAIR_H
