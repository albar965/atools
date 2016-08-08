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

/* Used to enable/disable loading of BGL objects/records */
enum BglObjectType
{
  UNKNOWN,
  AIRPORT, /* airport and all subrecords */
  RUNWAY, /* runway and all subrecords */
  HELIPAD,
  START, /* runway start position */
  APPROACH, /* approach and all subrecords */
  APPROACHLEG,
  COM,
  PARKING,
  ILS,
  VOR,
  NDB,
  WAYPOINT,
  BOUNDARY, /* airspace boundary and all subrecords */
  MARKER,
  APRON, /* apron and its vertices */
  APRON2, /* apron and its vertices2 lists */
  APRONLIGHT,
  FENCE, /* boundary and blast fences */
  TAXIWAY,
  VEHICLE, /* taxiway and parking for airport vehicles */
  TAXIWAY_RUNWAY, /* taxiway across runways */
  AIRWAY, /* all airway route processing */
  GEOMETRY /* apron and fence geometry */
};

QString bglObjectTypeToString(atools::fs::type::BglObjectType type);
atools::fs::type::BglObjectType stringToBglObjectType(const QString& typeStr);

}

class NavDatabaseProgress;

/*
 * Configuration options for NavDatabase reader class. Can be loaded from a settings file (.ini format)
 */
class NavDatabaseOptions
{
public:
  NavDatabaseOptions();

  /*
   * Fill this configuration object from a settings file.
   * See project "navdatareader" directory "resources/config/navdatareader.cfg" for an example.
   * Note that some settings still have to be set programmatically.
   *
   * @param settings path to the .ini file
   */
  void loadFromSettings(const QSettings& settings);

  /*
   * Set scenery.cfg file. This is mandatory.
   */
  void setSceneryFile(const QString& value)
  {
    sceneryFile = value;
  }

  /*
   * Set flight simulator basepath. This is mandatory.
   */
  void setBasepath(const QString& value)
  {
    basepath = value;
  }

  /*
   * Set verbose logging. This is only useful with small datasets. Default is false.
   */
  void setVerbose(bool value)
  {
    verbose = value;
  }

  /*
   * Set to true to execute airport deletion for add-on airports. Default is true.
   * Setting to false will result in duplicate airports in the database.
   */
  void setDeletes(bool value)
  {
    deletes = value;
  }

  /*
   * True: create a final report on database content. Default is false.
   */
  void setDatabaseReport(bool value)
  {
    databaseReport = value;
  }

  /*
   * true: Filter out dummy runways that were created for ATC and traffic. Default is true.
   */
  void setFilterOutDummyRunways(bool value)
  {
    filterRunways = value;
  }

  /*
   * Set to true to write incomplete objects like an ILS without runway. Default is true.
   */
  void setWriteIncompleteObjects(bool value)
  {
    incomplete = value;
  }

  /*
   * True: Commit after each database action. Default is false.
   * This slows down loading considerably. Only for debugging.
   */
  void setAutocommit(bool value)
  {
    autocommit = value;
  }

  /*
   * Fill the airway table and connect all waypoints of a route. Default is true.
   */
  void setResolveAirways(bool value)
  {
    resolveAirways = value;
  }

  /*
   * If true create all route_edge_* and route_node_* tables that are needed for flight plan creation
   */
  void setCreateRouteTables(bool value)
  {
    createRouteTables = value;
  }

  typedef std::function<bool (const atools::fs::NavDatabaseProgress&)> ProgressCallbackType;

  /* Set progress callback function/method */
  void setProgressCallback(ProgressCallbackType func);

  /* Exclude absolute directories just by path.  */
  void addToDirectoryExcludes(const QStringList& filter);

  /* Exclude absolute directories from add-on recognition */
  void addToAddonDirectoryExcludes(const QStringList& filter);

  // -------------------------------- getters below

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

  ProgressCallbackType getProgressCallback() const;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::NavDatabaseOptions& opts);

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
