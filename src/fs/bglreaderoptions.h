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

#ifndef ATOOLS_FS_BGLREADEROPTIONS_H
#define ATOOLS_FS_BGLREADEROPTIONS_H

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
  GEOMETRY /* apron and fence geometry */
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

  bool isCreateRouteTables() const
  {
    return createRouteTables;
  }

  bool isIncludedFilename(const QString& filename) const;
  bool isIncludedLocalPath(const QString& filepath) const;
  bool isIncludedAirportIdent(const QString& icao) const;
  bool isIncludedDirectory(const QString& filepath) const;

  bool isAddonLocalPath(const QString& filepath) const;
  bool isAddonDirectory(const QString& filepath) const;

  bool isIncludedBglObject(atools::fs::type::BglObjectType type) const;

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

  void setCreateRouteTables(bool value)
  {
    createRouteTables = value;
  }

  typedef std::function<bool (const atools::fs::BglReaderProgressInfo&)> ProgressCallbackType;

  void setProgressCallback(ProgressCallbackType func);
  ProgressCallbackType getProgressCallback() const;

  /* Exclude absolute directories */
  void addToDirectoryExcludes(const QStringList& filter);

  /* Exclude absolute directories from addon recognition */
  void addToAddonDirectoryExcludes(const QStringList& filter);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::BglReaderOptions& opts);

  void addToAddonFilterInclude(const QStringList& filter);
  void addToAddonFilterExclude(const QStringList& filter);
  void addToFilenameFilterInclude(const QStringList& filter);
  void addToFilenameFilterExclude(const QStringList& filter);
  void addToAirportIcaoFilterInclude(const QStringList& filter);
  void addToAirportIcaoFilterExclude(const QStringList& filter);
  void addToPathFilterInclude(const QStringList& filter);
  void addToPathFilterExclude(const QStringList& filter);
  void addToBglObjectFilterInclude(const QStringList& filters);
  void addToBglObjectFilterExclude(const QStringList& filters);

  void addToFilter(const QStringList& filters, QList<QRegExp>& filterList);
  bool includeObject(const QString& string, const QList<QRegExp>& filterListInc,
                     const QList<QRegExp>& filterListExcl) const;

  void addToBglObjectFilter(const QStringList& filters, QSet<atools::fs::type::BglObjectType>& filterList);
  QString adaptPath(const QString& filepath) const;
  QStringList fromNativeSeparators(const QStringList& paths) const;
  QString fromNativeSeparator(const QString& path) const;
  QStringList createFilterList(const QStringList& pathList);

  QString sceneryFile, basepath;
  bool verbose = false, deletes = true, filterRunways = true, incomplete = true, autocommit = false,
       resolveAirways = true, createRouteTables = true, databaseReport = false;

  QList<QRegExp> fileFiltersInc, pathFiltersInc, addonFiltersInc, airportIcaoFiltersInc,
                 fileFiltersExcl, pathFiltersExcl, addonFiltersExcl, airportIcaoFiltersExcl,
                 dirExcludes /* Not loaded from config file */,
                 addonDirExcludes /* Not loaded from config file */;
  QSet<atools::fs::type::BglObjectType> bglObjectTypeFiltersInc, bglObjectTypeFiltersExcl;
  ProgressCallbackType progressCallback = nullptr;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_BGLREADEROPTIONS_H
