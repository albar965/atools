/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_NAVDATABASEOPTIONS_H
#define ATOOLS_FS_NAVDATABASEOPTIONS_H

#include "fs/fspaths.h"

#include <functional>

#include <QSet>
#include <QFlags>
#include <QRegExp>
#include <QMap>

class QSettings;

namespace atools {
namespace fs {

class NavDatabaseProgress;

namespace type {

/* Used to enable/disable loading of BGL objects/records and files for X-Plane. */
enum NavDbObjectType
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
  TAXIWAY,
  VEHICLE, /* taxiway and parking for airport vehicles */
  TAXIWAY_RUNWAY, /* taxiway across runways */
  AIRWAY, /* all airway route processing */
  GEOMETRY /* apron geometry */
};

QString navDbObjectTypeToString(atools::fs::type::NavDbObjectType type);
atools::fs::type::NavDbObjectType stringToNavDbObjectType(const QString& typeStr);

enum OptionFlag
{
  NO_OPTION_FLAG = 0,
  /*
   * Set verbose logging. This is only useful with small datasets. Default is false.
   */
  VERBOSE = 1 << 0,

  /*
   * Set to true to execute airport deletion for add-on airports. Default is true.
   * Setting to false will result in duplicate airports in the database.
   */
  DELETES = 1 << 1,

  /*
   * Set to true to delete duplicates. Default is true.
   */
  DEDUPLICATE = 1 << 2,

  /*
   * true: Filter out dummy runways that were created for ATC and traffic. Default is true.
   */
  FILTER_RUNWAYS = 1 << 3,

  /*
   * Set to true to write incomplete objects like an ILS without runway. Default is true.
   */
  INCOMPLETE = 1 << 4,

  /*
   * True: Commit after each database action. Default is false.
   * This slows down loading considerably. Only for debugging.
   */
  AUTOCOMMIT = 1 << 5,

  /*
   * Fill the airway table and connect all waypoints of a route. Default is true.
   */
  RESOLVE_AIRWAYS = 1 << 6,

  /*
   * If true create all route_edge_* and route_node_* tables that are needed for flight plan creation
   */
  CREATE_ROUTE_TABLES = 1 << 7,

  /*
   * True: create a final report on database content. Default is false.
   */
  DATABASE_REPORT = 1 << 8,

  /* Reads all inactive scenery regions if set to true */
  READ_INACTIVE = 1 << 9,

  /* Reads all inactive scenery regions if set to true */
  READ_ADDON_XML = 1 << 10,

  /* Does a very basic validation and checks if most important tables are filled
   * True: Check for most important tables populated and throw exception if not.*/
  BASIC_VALIDATION = 1 << 11,

  /* Vaccuum database at end */
  VACUUM_DATABASE = 1 << 12,

  /* Run SQLite analyze command */
  ANALYZE_DATABASE = 1 << 13,

  /* Remove all indexes */
  DROP_INDEXES = 1 << 14,

  /* Remove all indexes */
  AIRPORT_VALIDATION = 1 << 15
};

Q_DECLARE_FLAGS(OptionFlags, OptionFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::type::OptionFlags);

}

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
  void loadFromSettings(QSettings& settings);

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

  void setMsfsCommunityPath(const QString& value)
  {
    msfsCommunityPath = value;
  }

  void setMsfsOfficialPath(const QString& value)
  {
    msfsOfficialPath = value;
  }

  /*
   * Set source database to copy from.
   */
  void setSourceDatabase(const QString& value)
  {
    sourceDatabase = value;
  }

  /*
   * Set verbose logging. This is only useful with small datasets. Default is false.
   */
  void setVerbose(bool value)
  {
    flags.setFlag(type::VERBOSE, value);
  }

  /*
   * Set to true to execute airport deletion for add-on airports. Default is true.
   * Setting to false will result in duplicate airports in the database.
   */
  void setDeletes(bool value)
  {
    flags.setFlag(type::DELETES, value);
  }

  /*
   * Set to true to delete duplicates. Default is true.
   */
  void setDeduplicate(bool value)
  {
    flags.setFlag(type::DEDUPLICATE, value);
  }

  /*
   * True: create a final report on database content. Default is false.
   */
  void setDatabaseReport(bool value)
  {
    flags.setFlag(type::DATABASE_REPORT, value);
  }

  /*
   * true: Filter out dummy runways that were created for ATC and traffic. Default is true.
   */
  void setFilterOutDummyRunways(bool value)
  {
    flags.setFlag(type::FILTER_RUNWAYS, value);
  }

  /*
   * Set to true to write incomplete objects like an ILS without runway. Default is true.
   */
  void setWriteIncompleteObjects(bool value)
  {
    flags.setFlag(type::INCOMPLETE, value);
  }

  /*
   * True: Commit after each database action. Default is false.
   * This slows down loading considerably. Only for debugging.
   */
  void setAutocommit(bool value)
  {
    flags.setFlag(type::AUTOCOMMIT, value);
  }

  /*
   * Fill the airway table and connect all waypoints of a route. Default is true.
   */
  void setResolveAirways(bool value)
  {
    flags.setFlag(type::RESOLVE_AIRWAYS, value);
  }

  /*
   * If true create all route_edge_* and route_node_* tables that are needed for flight plan creation
   */
  void setCreateRouteTables(bool value)
  {
    flags.setFlag(type::CREATE_ROUTE_TABLES, value);
  }

  /* Reads all inactive scenery regions if set to true */
  void setReadInactive(bool value)
  {
    flags.setFlag(type::READ_INACTIVE, value);
  }

  /* Reads all inactive scenery regions if set to true */
  void setReadAddOnXml(bool value)
  {
    flags.setFlag(type::READ_ADDON_XML, value);
  }

  typedef std::function<bool (const atools::fs::NavDatabaseProgress&)> ProgressCallbackType;

  /* Set progress callback function/method */
  void setProgressCallback(ProgressCallbackType func);

  /* Exclude absolute directories and file paths. Used by the GUI options dialog. */
  void addToDirectoryExcludes(const QStringList& filter);
  void addToFilePathExcludes(const QStringList& filter);

  /* Exclude absolute directories from add-on recognition. Used by the GUI options dialog. */
  void addToAddonDirectoryExcludes(const QStringList& filter);

  void setSimulatorType(const atools::fs::FsPaths::SimulatorType& value)
  {
    simulatorType = value;
  }

  // -------------------------------- getters below

  const QString& getBasepath() const
  {
    return basepath;
  }

  /* .../Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Community */
  const QString& getMsfsCommunityPath() const
  {
    return msfsCommunityPath;
  }

  /* .../Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore */
  const QString& getMsfsOfficialPath() const
  {
    return msfsOfficialPath;
  }

  const QString& getSourceDatabase() const
  {
    return sourceDatabase;
  }

  bool isDeletes() const
  {
    return flags.testFlag(type::DELETES);
  }

  bool isDeduplicate() const
  {
    return flags.testFlag(type::DEDUPLICATE);
  }

  bool isFilterRunways() const
  {
    return flags.testFlag(type::FILTER_RUNWAYS);
  }

  const QString& getSceneryFile() const
  {
    return sceneryFile;
  }

  bool isVerbose() const
  {
    return flags.testFlag(type::VERBOSE);
  }

  bool isAutocommit() const
  {
    return flags.testFlag(type::AUTOCOMMIT);
  }

  bool isIncomplete() const
  {
    return flags.testFlag(type::INCOMPLETE);
  }

  bool isDatabaseReport() const
  {
    return flags.testFlag(type::DATABASE_REPORT);
  }

  bool isVacuumDatabase() const
  {
    return flags.testFlag(type::VACUUM_DATABASE);
  }

  bool isAnalyzeDatabase() const
  {
    return flags.testFlag(type::ANALYZE_DATABASE);
  }

  bool isDropIndexes() const
  {
    return flags.testFlag(type::DROP_INDEXES);
  }

  bool isBasicValidation() const
  {
    return flags.testFlag(type::BASIC_VALIDATION);
  }

  bool isAirportValidation() const
  {
    return flags.testFlag(type::AIRPORT_VALIDATION);
  }

  bool isResolveAirways() const
  {
    return flags.testFlag(type::RESOLVE_AIRWAYS);
  }

  bool isCreateRouteTables() const
  {
    return flags.testFlag(type::CREATE_ROUTE_TABLES);
  }

  bool isReadInactive() const
  {
    return flags.testFlag(type::READ_INACTIVE);
  }

  bool isReadAddOnXml() const
  {
    return flags.testFlag(type::READ_ADDON_XML);
  }

  /* Pure file name */
  bool isIncludedFilename(const QString& filename) const;

  /* Path relative to base */
  bool isIncludedLocalPath(const QString& filepath) const;
  bool isIncludedAirportIdent(const QString& icao) const;

  /* Options that are not saved with the object */
  bool isIncludedDirectory(const QString& dirpath) const;
  bool isIncludedFilePath(const QString& filepath) const;

  /* If true scenery will be added to end of list */
  bool isHighPriority(const QString& filepath) const;

  bool isAddonLocalPath(const QString& filepath) const;
  bool isAddonDirectory(const QString& filepath) const;

  bool isIncludedNavDbObject(atools::fs::type::NavDbObjectType type) const;

  ProgressCallbackType getProgressCallback() const;

  atools::fs::FsPaths::SimulatorType getSimulatorType() const
  {
    return simulatorType;
  }

  const atools::fs::type::OptionFlags& getFlags() const
  {
    return flags;
  }

  void setFlag(const atools::fs::type::OptionFlags& value, bool on = true)
  {
    if(on)
      flags |= value;
    else
      flags &= ~value;
  }

  /* Maps table name to minimum number of rows */
  const QMap<QString, int>& getBasicValidationTables() const
  {
    return basicValidationTables;
  }

  void setBasicValidationTables(const QMap<QString, int>& value)
  {
    basicValidationTables = value;
  }

  /* Language for MSFS airport, city and country names like "en-US" or "de-DE" */
  QString getLanguage() const
  {
    return language;
  }

  void setLanguage(const QString& value);

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

  void addToHighPriorityFiltersInc(const QStringList& filters);

  void addToFilter(const QStringList& filters, QList<QRegExp>& filterList);
  bool includeObject(const QString& string, const QList<QRegExp>& filterListInc,
                     const QList<QRegExp>& filterListExcl) const;

  void addToBglObjectFilter(const QStringList& filters, QSet<atools::fs::type::NavDbObjectType>& filterList);
  QString adaptPath(const QString& filepath) const;
  QStringList fromNativeSeparators(const QStringList& paths) const;
  QString fromNativeSeparator(const QString& path) const;
  QStringList createFilterList(const QStringList& pathList);

  QString sceneryFile, basepath, msfsCommunityPath, msfsOfficialPath, sourceDatabase, language = "en-US";

  atools::fs::type::OptionFlags flags;

  QMap<QString, int> basicValidationTables;
  QList<QRegExp> fileFiltersInc, pathFiltersInc, addonFiltersInc, airportIcaoFiltersInc,
                 fileFiltersExcl, pathFiltersExcl, addonFiltersExcl, airportIcaoFiltersExcl,
                 highPriorityFiltersInc,
                 dirExcludesGui /* Not loaded from config file */,
                 filePathExcludesGui /* Not loaded from config file */,
                 addonDirExcludes /* Not loaded from config file */;
  QSet<atools::fs::type::NavDbObjectType> navDbObjectTypeFiltersInc, navDbObjectTypeFiltersExcl;
  ProgressCallbackType progressCallback = nullptr;

  atools::fs::FsPaths::SimulatorType simulatorType = atools::fs::FsPaths::FSX;
};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASEOPTIONS_H
