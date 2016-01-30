/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef BGLREADEROPTIONS_H_
#define BGLREADEROPTIONS_H_

#include <QRegularExpression>
#include <QSet>

namespace atools {
namespace fs {

namespace type {
enum BglObjectType
{
  UNKNOWN,
  AIRPORT, // ok
  RUNWAY, // ok
  APPROACH, // ok
  COM, // ok
  PARKING, // ok
  ILS, // ok
  VOR, // ok
  NDB, // ok
  WAYPOINT, // ok
  MARKER, // ok
  ROUTE
};

QString bglObjectTypeToString(atools::fs::type::BglObjectType type);
atools::fs::type::BglObjectType stringToBglObjectType(const QString& typeStr);

}

class BglReaderOptions
{
public:
  BglReaderOptions();

  const QString& getBasepath() const
  {
    return basepath;
  }

  bool isDeletes() const
  {
    return deletes;
  }

  bool isFilterRunways() const
  {
    return filterRunways;
  }

  const QString& getSceneryFile() const
  {
    return sceneryFile;
  }

  bool isVerbose() const
  {
    return verbose;
  }

  bool isDebugAutocommit() const
  {
    return debugAutocommit;
  }

  bool isIncomplete() const
  {
    return incomplete;
  }

  bool includeFilename(const QString& filename) const;
  bool includePath(const QString& filename) const;
  bool includeAirport(const QString& icao) const;
  bool includeBglObject(atools::fs::type::BglObjectType type) const;

  void setSceneryFile(const QString& value)
  {
    sceneryFile = value;
  }

  void setBasepath(const QString& value)
  {
    basepath = value;
  }

  void setVerbose(bool value)
  {
    verbose = value;
  }

  void setFilenameFilterInc(const QStringList& filter);
  void setFilenameFilterExcl(const QStringList& filter);
  void setAirportIcaoFilterInc(const QStringList& filter);
  void setAirportIcaoFilterExcl(const QStringList& filter);
  void setPathFilterInc(const QStringList& filter);
  void setPathFilterExcl(const QStringList& filter);
  void setBglObjectFilterInc(const QStringList& filters);
  void setBglObjectFilterExcl(const QStringList& filters);

  void setDeletes(bool value)
  {
    deletes = value;
  }

  void setFilterRunways(bool value)
  {
    filterRunways = value;
  }

  void setIncomplete(bool value)
  {
    incomplete = value;
  }

  void setDebugAutocommit(bool value)
  {
    debugAutocommit = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::BglReaderOptions& opts);

  void setFilter(const QStringList& filters, QList<QRegExp>& filterList);
  bool includeObject(const QString& icao,
                     const QList<QRegExp>& filterListInc,
                     const QList<QRegExp>& filterListExcl) const;

  void setBglObjectFilter(const QStringList& filters, QSet<atools::fs::type::BglObjectType>& filterList);

  QString sceneryFile, basepath;
  bool verbose, deletes, filterRunways, incomplete, debugAutocommit;

  QList<QRegExp> fileFiltersInc, pathFiltersInc, airportIcaoFiltersInc,
                 fileFiltersExcl, pathFiltersExcl, airportIcaoFiltersExcl;
  QSet<atools::fs::type::BglObjectType> bglObjectTypeFiltersInc, bglObjectTypeFiltersExcl;

};

} // namespace fs
} // namespace atools

#endif /* BGLREADEROPTIONS_H_ */
