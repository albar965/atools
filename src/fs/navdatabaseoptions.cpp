/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/navdatabaseoptions.h"

#include "atools.h"

#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QDir>
#include <QSettings>

namespace atools {
namespace fs {

const static QList<QRegExp> EMPTY_REGEXP_LIST;

void NavDatabaseOptions::setLanguage(const QString& lang)
{
  if(lang.isEmpty())
    language = "en-US";
  else
  {
    language = lang;
    language.replace('_', '-');

    if(language.contains('-'))
      // Make second section upper case
      language = language.section('-', 0, 0) + "-" + language.section('-', 1, 1).toUpper();
  }
}

void NavDatabaseOptions::addIncludeGui(const QFileInfo& path)
{
  if(path.exists() && path.isDir())
    dirIncludesGui.append(atools::canonicalFilePath(path));
  else
    qWarning() << Q_FUNC_INFO << "Inclusion path does not exist or is no dir" << path;
}

void NavDatabaseOptions::addExcludeGui(const QFileInfo& path)
{
  if(path.exists())
  {
    if(path.isDir())
      addToFilter(createDirFilter(atools::canonicalFilePath(path)), dirExcludesGui);
    else if(path.isFile())
      addToFilter(atools::canonicalFilePath(path), fileExcludesGui);
  }
  else
    qWarning() << Q_FUNC_INFO << "Exclusion path does not exist" << path;
}

void NavDatabaseOptions::addAddonExcludeGui(const QFileInfo& path)
{
  if(path.exists())
  {
    if(path.isDir())
      addToFilter(createDirFilter(atools::canonicalFilePath(path)), dirAddonExcludesGui);
    else if(path.isFile())
      addToFilter(atools::canonicalFilePath(path), fileAddonExcludesGui);
  }
  else
    qWarning() << Q_FUNC_INFO << "Addon exclusion path does not exist" << path;
}

bool NavDatabaseOptions::isIncludedLocalPath(const QString& filepath) const
{
  return includeObject(adaptDir(filepath), pathFiltersInc, pathFiltersExcl);
}

bool NavDatabaseOptions::isAddonLocalPath(const QString& filepath) const
{
  return includeObject(adaptDir(filepath), addonFiltersInc, addonFiltersExcl);
}

bool NavDatabaseOptions::isIncludedGui(const QFileInfo& filepath) const
{
  return includedGui(filepath, fileExcludesGui, dirExcludesGui);
}

bool NavDatabaseOptions::isAddonGui(const QFileInfo& filepath) const
{
  return includedGui(filepath, dirAddonExcludesGui, fileAddonExcludesGui);
}

bool NavDatabaseOptions::includedGui(const QFileInfo& path, const QList<QRegExp>& fileExclude, const QList<QRegExp>& dirExclude) const
{
  if(fileExclude.isEmpty() && dirExclude.isEmpty())
    return true;

  if(path.isDir())
  {
    if(!includeObject(adaptDir(atools::canonicalFilePath(path)), EMPTY_REGEXP_LIST, dirExclude))
      return false;
  }
  else if(path.isFile())
  {
    // First check path to file
    if(!includeObject(adaptDir(atools::canonicalPath(path)), EMPTY_REGEXP_LIST, dirExclude))
      return false;

    // Check file name
    if(!includeObject(atools::canonicalFilePath(path), EMPTY_REGEXP_LIST, fileExclude))
      return false;
  }

  return true;
}

bool NavDatabaseOptions::isHighPriority(const QString& filepath) const
{
  if(highPriorityFiltersInc.isEmpty())
    return false;

  return includeObject(adaptDir(filepath), highPriorityFiltersInc, EMPTY_REGEXP_LIST);
}

bool NavDatabaseOptions::isIncludedFilename(const QString& filename) const
{
  QString fn = QFileInfo(filename).fileName();
  return includeObject(fn, fileFiltersInc, fileFiltersExcl);
}

bool NavDatabaseOptions::isIncludedAirportIdent(const QString& icao) const
{
  return includeObject(icao, airportIcaoFiltersInc, airportIcaoFiltersExcl);
}

void NavDatabaseOptions::addToFilenameFilterInclude(const QStringList& filter)
{
  addToFilterList(filter, fileFiltersInc);
}

void NavDatabaseOptions::addToAirportIcaoFilterInclude(const QStringList& filter)
{
  addToFilterList(filter, airportIcaoFiltersInc);
}

void NavDatabaseOptions::addToPathFilterInclude(const QStringList& filter)
{
  addToFilterList(fromNativeSeparatorList(filter), pathFiltersInc);
}

void NavDatabaseOptions::addToAddonFilterInclude(const QStringList& filter)
{
  addToFilterList(fromNativeSeparatorList(filter), addonFiltersInc);
}

void NavDatabaseOptions::addToFilenameFilterExclude(const QStringList& filter)
{
  addToFilterList(filter, fileFiltersExcl);
}

void NavDatabaseOptions::addToAirportIcaoFilterExclude(const QStringList& filter)
{
  addToFilterList(filter, airportIcaoFiltersExcl);
}

void NavDatabaseOptions::addToPathFilterExclude(const QStringList& filter)
{
  addToFilterList(fromNativeSeparatorList(filter), pathFiltersExcl);
}

void NavDatabaseOptions::addToAddonFilterExclude(const QStringList& filter)
{
  addToFilterList(fromNativeSeparatorList(filter), addonFiltersExcl);
}

void NavDatabaseOptions::addToNavDbObjectFilterInclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, navDbObjectTypeFiltersInc);
}

void NavDatabaseOptions::addToNavDbObjectFilterExclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, navDbObjectTypeFiltersExcl);
}

void NavDatabaseOptions::addToHighPriorityFiltersInc(const QStringList& filters)
{
  addToFilterList(filters, highPriorityFiltersInc);
}

void NavDatabaseOptions::addToBglObjectFilter(const QStringList& filters,
                                              QSet<atools::fs::type::NavDbObjectType>& filterList)
{
  for(const QString& f : filters)
  {
    if(!f.isEmpty())
      filterList.insert(type::stringToNavDbObjectType(f));
  }
}

bool NavDatabaseOptions::isIncludedNavDbObject(type::NavDbObjectType type) const
{
  if(navDbObjectTypeFiltersInc.isEmpty() && navDbObjectTypeFiltersExcl.isEmpty())
    return true;

  bool exFound = navDbObjectTypeFiltersExcl.contains(type);

  if(navDbObjectTypeFiltersInc.isEmpty())
    return !exFound;
  else
  {
    bool incFound = navDbObjectTypeFiltersInc.contains(type);

    if(navDbObjectTypeFiltersExcl.isEmpty())
      return incFound;
    else
      return incFound && !exFound;
  }
}

QString NavDatabaseOptions::createDirFilter(const QString& path)
{
  QDir dir(path);
  QString filter = dir.absolutePath().trimmed().replace('\\', '/');
  if(!filter.endsWith("/"))
    filter.append("/");
  filter.append("*");
  return filter;
}

void NavDatabaseOptions::loadFromSettings(QSettings& settings)
{
  setReadInactive(settings.value("Options/IgnoreInactive", false).toBool());
  setVerbose(settings.value("Options/Verbose", false).toBool());
  setResolveAirways(settings.value("Options/ResolveRoutes", true).toBool());
  setLanguage(settings.value("Options/MsfsAirportLanguage", "en-US").toString());
  setDatabaseReport(settings.value("Options/DatabaseReport", true).toBool());
  setDeletes(settings.value("Options/ProcessDelete", true).toBool());
  setDeduplicate(settings.value("Options/Deduplicate", true).toBool());
  setFilterOutDummyRunways(settings.value("Options/FilterRunways", true).toBool());
  setWriteIncompleteObjects(settings.value("Options/SaveIncomplete", true).toBool());
  setAutocommit(settings.value("Options/Autocommit", false).toBool());
  setFlag(type::BASIC_VALIDATION, settings.value("Options/BasicValidation", false).toBool());
  setFlag(type::AIRPORT_VALIDATION, settings.value("Options/AirportValidation", false).toBool());
  setFlag(type::VACUUM_DATABASE, settings.value("Options/VacuumDatabase", true).toBool());
  setFlag(type::ANALYZE_DATABASE, settings.value("Options/AnalyzeDatabase", true).toBool());
  setFlag(type::DROP_INDEXES, settings.value("Options/DropAllIndexes", false).toBool());
  setFlag(type::DROP_TEMP_TABLES, settings.value("Options/DropTempTables", true).toBool());

  setSimConnectAirportFetchDelay(settings.value("Options/SimConnectAirportFetchDelay", 100).toInt());
  setSimConnectNavaidFetchDelay(settings.value("Options/SimConnectNavaidFetchDelay", 50).toInt());
  setSimConnectBatchSize(settings.value("Options/SimConnectBatchSize", 2000).toInt());
  setSimConnectLoadDisconnected(settings.value("Options/SimConnectLoadDisconnected", true).toBool());

  addToHighPriorityFiltersInc(settings.value("Filter/IncludeHighPriorityFilter").toStringList());
  addToFilenameFilterInclude(settings.value("Filter/IncludeFilenames").toStringList());
  addToFilenameFilterExclude(settings.value("Filter/ExcludeFilenames").toStringList());
  addToPathFilterInclude(settings.value("Filter/IncludePathFilter").toStringList());
  addToPathFilterExclude(settings.value("Filter/ExcludePathFilter").toStringList());
  addToAddonFilterInclude(settings.value("Filter/IncludeAddonPathFilter").toStringList());
  addToAddonFilterExclude(settings.value("Filter/ExcludeAddonPathFilter").toStringList());
  addToAirportIcaoFilterInclude(settings.value("Filter/IncludeAirportIcaoFilter").toStringList());
  addToAirportIcaoFilterExclude(settings.value("Filter/ExcludeAirportIcaoFilter").toStringList());
  addToNavDbObjectFilterInclude(settings.value("Filter/IncludeBglObjectFilter").toStringList());
  addToNavDbObjectFilterExclude(settings.value("Filter/ExcludeBglObjectFilter").toStringList());

  settings.beginGroup("BasicValidationTables");

  QString simStr = simulatorType == FsPaths::DFD ? "DFD" : FsPaths::typeToShortName(simulatorType);

  const QStringList childKeys = settings.childKeys();
  for(const QString& key : childKeys)
  {
    // Addd keys without prefix or keys with matching prefix
    if(!key.contains('.') || key.section('.', 0, 0) == simStr)
      basicValidationTables.insert(key.section('.', 1, 1), settings.value(key).toInt());
  }

  settings.endGroup();
}

bool NavDatabaseOptions::includeObject(const QString& string, const QList<QRegExp>& filterListInc,
                                       const QList<QRegExp>& filterListExcl) const
{
  if(filterListInc.isEmpty() && filterListExcl.isEmpty())
    return true;

  bool excludeMatched = false;
  for(const QRegExp& iter : filterListExcl)
  {
    if(iter.exactMatch(string))
    {
      excludeMatched = true;
      break;
    }
  }

  if(filterListInc.isEmpty())
    // No include filters - let exclude filter decide
    return !excludeMatched;
  else
  {
    bool includeMatched = false;
    for(const QRegExp& iter : filterListInc)
    {
      if(iter.exactMatch(string))
      {
        includeMatched = true;
        break;
      }
    }

    if(filterListExcl.isEmpty())
      // No exclude filters - let include filter decide
      return includeMatched;
    else
      return includeMatched && !excludeMatched;
  }
}

void NavDatabaseOptions::addToFilterList(const QStringList& filters, QList<QRegExp>& filterList)
{
  for(const QString& filter : filters)
    addToFilter(filter, filterList);
}

void NavDatabaseOptions::addToFilter(const QString& filter, QList<QRegExp>& filterList)
{
  if(!filter.isEmpty())
    filterList.append(QRegExp(filter.trimmed(), Qt::CaseInsensitive, QRegExp::Wildcard));
}

QString NavDatabaseOptions::adaptDir(const QString& filepath) const
{
  // make sure that backslashes are replaced and path is suffixed with a slash
  QString newFilename = atools::cleanPath(filepath);
  if(!filepath.endsWith('/'))
    newFilename.append('/');

  return newFilename;
}

QStringList NavDatabaseOptions::fromNativeSeparatorList(const QStringList& paths) const
{
  QStringList retval;
  for(const QString& p : paths)
    retval.append(atools::cleanPath(p));
  return retval;
}

QString patternStr(const QList<QRegExp>& list)
{
  QStringList retval;
  for(const QRegExp& regexp : list)
    retval.append(regexp.pattern());
  return retval.join(", ");
}

QDebug operator<<(QDebug out, const NavDatabaseOptions& opts)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "NavDatabaseOptions[flags " << opts.flags;

  out << ", language \"" << opts.language << "\"";
  out << ", SimConnectAirportFetchDelay \"" << opts.simConnectAirportFetchDelay << "\"";
  out << ", SimConnectNavaidFetchDelay \"" << opts.simConnectNavaidFetchDelay << "\"";
  out << ", SimConnectBatchSize \"" << opts.simConnectBatchSize << "\"";
  out << ", SimConnectLoadDisconnected \"" << opts.simConnectLoadDisconnected << "\"";
  out << ", sceneryFile \"" << opts.sceneryFile << "\"";
  out << ", basepath \"" << opts.basepath << "\"";
  out << ", msfsCommunityPath \"" << opts.msfsCommunityPath << "\"";
  out << ", msfsOfficialPath \"" << opts.msfsOfficialPath << "\"";
  out << ", sourceDatabase \"" << opts.sourceDatabase << "\"";
  out << ", basicValidationTables \"" << opts.basicValidationTables << "\"";
  out << ", fileFiltersInc [" << patternStr(opts.fileFiltersInc) << "]";
  out << ", fileFiltersExcl [" << patternStr(opts.fileFiltersExcl) << "]";
  out << ", pathFiltersInc [" << patternStr(opts.pathFiltersInc) << "]";
  out << ", pathFiltersExcl [" << patternStr(opts.pathFiltersExcl) << "]";
  out << ", airportIcaoFiltersInc [" << patternStr(opts.airportIcaoFiltersInc) << "]";
  out << ", airportIcaoFiltersExcl [" << patternStr(opts.airportIcaoFiltersExcl) << "]";
  out << ", addonFiltersInc [" << patternStr(opts.addonFiltersInc) << "]";
  out << ", addonFiltersExcl [" << patternStr(opts.addonFiltersExcl) << "]";
  out << ", highPriorityFiltersInc [" << patternStr(opts.highPriorityFiltersInc) << "]";
  out << ", dirIncludesGui [" << opts.dirIncludesGui << "]";
  out << ", dirExcludesGui [" << patternStr(opts.dirExcludesGui) << "]";
  out << ", fileExcludesGui [" << patternStr(opts.fileExcludesGui) << "]";
  out << ", dirAddonExcludesGui [" << patternStr(opts.dirAddonExcludesGui) << "]";
  out << ", fileAddonExcludesGui [" << patternStr(opts.fileAddonExcludesGui) << "]";

  out << ", navDbObjectTypeFiltersInc [";
  for(type::NavDbObjectType type : opts.navDbObjectTypeFiltersInc)
    out << type::navDbObjectTypeToString(type) << ", ";
  out << "]";

  out << ", navDbObjectTypeFiltersExcl [";
  for(type::NavDbObjectType type : opts.navDbObjectTypeFiltersExcl)
    out << type::navDbObjectTypeToString(type) << ", ";
  out << "]";

  out << "]";
  return out;
}

QString type::navDbObjectTypeToString(type::NavDbObjectType type)
{
  switch(type)
  {
    case AIRPORT:
      return "AIRPORT";

    case RUNWAY:
      return "RUNWAY";

    case HELIPAD:
      return "HELIPAD";

    case START:
      return "START";

    case APPROACH:
      return "APPROACH";

    case APPROACHLEG:
      return "APPROACHLEG";

    case COM:
      return "COM";

    case PARKING:
      return "PARKING";

    case ILS:
      return "ILS";

    case VOR:
      return "VOR";

    case NDB:
      return "NDB";

    case WAYPOINT:
      return "WAYPOINT";

    case BOUNDARY:
      return "BOUNDARY";

    case MARKER:
      return "MARKER";

    case AIRWAY:
      return "AIRWAY";

    case APRON:
      return "APRON";

    case APRON2:
      return "APRON2";

    case TAXIWAY:
      return "TAXIWAY";

    case GEOMETRY:
      return "GEOMETRY";

    case UNKNOWN:
      return "UNKNWON";
  }
  return "UNKNWON";
}

type::NavDbObjectType type::stringToNavDbObjectType(const QString& typeStr)
{
  if(typeStr == "AIRPORT")
    return AIRPORT;
  else if(typeStr == "RUNWAY")
    return RUNWAY;
  else if(typeStr == "HELIPAD")
    return HELIPAD;
  else if(typeStr == "START")
    return START;
  else if(typeStr == "APPROACH")
    return APPROACH;
  else if(typeStr == "APPROACHLEG")
    return APPROACHLEG;
  else if(typeStr == "COM")
    return COM;
  else if(typeStr == "PARKING")
    return PARKING;
  else if(typeStr == "ILS")
    return ILS;
  else if(typeStr == "VOR")
    return VOR;
  else if(typeStr == "NDB")
    return NDB;
  else if(typeStr == "WAYPOINT")
    return WAYPOINT;
  else if(typeStr == "BOUNDARY")
    return BOUNDARY;
  else if(typeStr == "MARKER")
    return MARKER;
  else if(typeStr == "APRON")
    return APRON;
  else if(typeStr == "APRON2")
    return APRON2;
  else if(typeStr == "TAXIWAY")
    return TAXIWAY;
  else if(typeStr == "AIRWAY")
    return AIRWAY;
  else if(typeStr == "GEOMETRY")
    return GEOMETRY;
  else
    return UNKNOWN;
}

} // namespace fs
} // namespace atools
