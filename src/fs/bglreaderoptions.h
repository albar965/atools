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

#include <functional>
#include <QSet>
#include <QRegExp>

class QSettings;
class QStringList;

namespace atools {
namespace fs {

namespace type {
/* Used to enable/disable loading of objects */
enum BglObjectType
{
  UNKNOWN,
  AIRPORT,
  RUNWAY,
  HELIPAD,
  START,
  APPROACH,
  APPROACHLEG,
  COM,
  PARKING,
  ILS,
  VOR,
  NDB,
  WAYPOINT,
  BOUNDARY,
  MARKER,
  APRON,
  APRON2,
  APRONLIGHT,
  FENCE,
  TAXIWAY,
  AIRWAY,
  GEOMETRY
};

QString bglObjectTypeToString(atools::fs::type::BglObjectType type);
atools::fs::type::BglObjectType stringToBglObjectType(const QString& typeStr);

}

class BglReaderProgressInfo;

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

  bool isAutocommit() const
  {
    return autocommit;
  }

  bool isIncomplete() const
  {
    return incomplete;
  }

  bool isDatabaseReport() const
  {
    return databaseReport;
  }

  bool isResolveAirways() const
  {
    return resolveAirways;
  }

  bool includeFilename(const QString& filename) const;
  bool includePath(const QString& filepath) const;
  bool includeAirport(const QString& icao) const;
  bool includeBglObject(atools::fs::type::BglObjectType type) const;
  bool isAddonPath(const QString& filepath) const;

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

  void setDeletes(bool value)
  {
    deletes = value;
  }

  void setDatabaseReport(bool value)
  {
    databaseReport = value;
  }

  void setFilterRunways(bool value)
  {
    filterRunways = value;
  }

  void setIncomplete(bool value)
  {
    incomplete = value;
  }

  void setAutocommit(bool value)
  {
    autocommit = value;
  }

  void loadFromSettings(const QSettings& settings);

  void setResolveAirways(bool value)
  {
    resolveAirways = value;
  }

  typedef std::function<bool (const atools::fs::BglReaderProgressInfo&)> ProgressCallbackType;

  void setProgressCallback(ProgressCallbackType func);
  ProgressCallbackType getProgressCallback() const;

private:
  void setAddonFilterInc(const QStringList& filter);
  void setAddonFilterExcl(const QStringList& filter);
  void setFilenameFilterInc(const QStringList& filter);
  void setFilenameFilterExcl(const QStringList& filter);
  void setAirportIcaoFilterInc(const QStringList& filter);
  void setAirportIcaoFilterExcl(const QStringList& filter);
  void setPathFilterInc(const QStringList& filter);
  void setPathFilterExcl(const QStringList& filter);
  void setBglObjectFilterInc(const QStringList& filters);
  void setBglObjectFilterExcl(const QStringList& filters);

  friend QDebug operator<<(QDebug out, const atools::fs::BglReaderOptions& opts);

  void setFilter(const QStringList& filters, QList<QRegExp>& filterList);
  bool includeObject(const QString& icao, const QList<QRegExp>& filterListInc,
                     const QList<QRegExp>& filterListExcl) const;

  void setBglObjectFilter(const QStringList& filters, QSet<atools::fs::type::BglObjectType>& filterList);

  QString sceneryFile, basepath;
  bool verbose = false, deletes = true, filterRunways = true, incomplete = true, autocommit = false,
       resolveAirways = true, databaseReport = false;

  QList<QRegExp> fileFiltersInc, pathFiltersInc, addonFiltersInc, airportIcaoFiltersInc,
                 fileFiltersExcl, pathFiltersExcl, addonFiltersExcl, airportIcaoFiltersExcl;
  QSet<atools::fs::type::BglObjectType> bglObjectTypeFiltersInc, bglObjectTypeFiltersExcl;
  ProgressCallbackType progressCallback = nullptr;

  QString adaptPath(const QString& filepath) const;

  QStringList toNativeSeparators(const QStringList& paths) const;
  QString toNativeSeparator(const QString& path) const;

};

} // namespace fs
} // namespace atools

#endif /* BGLREADEROPTIONS_H_ */
