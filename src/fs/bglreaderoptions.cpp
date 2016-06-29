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

#include "fs/bglreaderoptions.h"
#include "logging/loggingdefs.h"
#include "scenery/sceneryarea.h"

#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QDir>
#include <QSettings>

namespace atools {
namespace fs {

BglReaderOptions::BglReaderOptions()
{
}

void BglReaderOptions::setProgressCallback(ProgressCallbackType func)
{
  progressCallback = func;
}

BglReaderOptions::ProgressCallbackType BglReaderOptions::getProgressCallback() const
{
  return progressCallback;
}

void BglReaderOptions::addToDirectoryExcludes(const QStringList& filter)
{
  addToFilter(createFilterList(filter), dirExcludes);
}

void BglReaderOptions::addToAddonDirectoryExcludes(const QStringList& filter)
{
  addToFilter(createFilterList(filter), addonDirExcludes);
}

bool BglReaderOptions::isIncludedLocalPath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), pathFiltersInc, pathFiltersExcl);
}

bool BglReaderOptions::isAddonLocalPath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), addonFiltersInc, addonFiltersExcl);
}

bool BglReaderOptions::isIncludedDirectory(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), QList<QRegExp>(), dirExcludes);
}

bool BglReaderOptions::isAddonDirectory(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), QList<QRegExp>(), addonDirExcludes);
}

bool BglReaderOptions::isIncludedFilename(const QString& filename) const
{
  QString fn = QFileInfo(filename).fileName();
  return includeObject(fn, fileFiltersInc, fileFiltersExcl);
}

bool BglReaderOptions::isIncludedAirportIdent(const QString& icao) const
{
  return includeObject(icao, airportIcaoFiltersInc, airportIcaoFiltersExcl);
}

void BglReaderOptions::addToFilenameFilterInclude(const QStringList& filter)
{
  addToFilter(filter, fileFiltersInc);
}

void BglReaderOptions::addToAirportIcaoFilterInclude(const QStringList& filter)
{
  addToFilter(filter, airportIcaoFiltersInc);
}

void BglReaderOptions::addToPathFilterInclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), pathFiltersInc);
}

void BglReaderOptions::addToAddonFilterInclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), addonFiltersInc);
}

void BglReaderOptions::addToFilenameFilterExclude(const QStringList& filter)
{
  addToFilter(filter, fileFiltersExcl);
}

void BglReaderOptions::addToAirportIcaoFilterExclude(const QStringList& filter)
{
  addToFilter(filter, airportIcaoFiltersExcl);
}

void BglReaderOptions::addToPathFilterExclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), pathFiltersExcl);
}

void BglReaderOptions::addToAddonFilterExclude(const QStringList& filter)
{
  addToFilter(fromNativeSeparators(filter), addonFiltersExcl);
}

void BglReaderOptions::addToBglObjectFilterInclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, bglObjectTypeFiltersInc);
}

void BglReaderOptions::addToBglObjectFilterExclude(const QStringList& filters)
{
  addToBglObjectFilter(filters, bglObjectTypeFiltersExcl);
}

void BglReaderOptions::addToBglObjectFilter(const QStringList& filters,
                                            QSet<atools::fs::type::BglObjectType>& filterList)
{
  for(const QString& f : filters)
    if(!f.isEmpty())
      filterList.insert(type::stringToBglObjectType(f));
}

bool BglReaderOptions::isIncludedBglObject(type::BglObjectType type) const
{
  if(bglObjectTypeFiltersInc.isEmpty() && bglObjectTypeFiltersExcl.isEmpty())
    return true;

  bool exFound = bglObjectTypeFiltersExcl.contains(type);

  if(bglObjectTypeFiltersInc.isEmpty())
    return !exFound;
  else
  {
    bool incFound = bglObjectTypeFiltersInc.contains(type);

    if(bglObjectTypeFiltersExcl.isEmpty())
      return incFound;
    else
      return incFound && !exFound;
  }
}

QStringList BglReaderOptions::createFilterList(const QStringList& pathList)
{
  QStringList retval;

  for(const QString& path : pathList)
  {
    QDir dir(path);
    QString newPath = dir.absolutePath().trimmed().replace("\\", "/");
    if(!newPath.endsWith("/"))
      newPath.append("/");
    newPath.append("*");
    retval.append(newPath);
  }
  return retval;
}

void BglReaderOptions::loadFromSettings(const QSettings& settings)
{
  setVerbose(settings.value("Options/Verbose", false).toBool());
  setResolveAirways(settings.value("Options/ResolveAirways", true).toBool());
  setCreateRouteTables(settings.value("Options/CreateRouteTables", false).toBool());
  setDatabaseReport(settings.value("Options/DatabaseReport", true).toBool());
  setDeletes(settings.value("Options/ProcessDelete", true).toBool());
  setFilterRunways(settings.value("Options/FilterRunways", true).toBool());
  setIncomplete(settings.value("Options/SaveIncomplete", true).toBool());
  setAutocommit(settings.value("Options/Autocommit", false).toBool());

  addToFilenameFilterInclude(settings.value("Filter/IncludeFilenames").toStringList());
  addToFilenameFilterExclude(settings.value("Filter/ExcludeFilenames").toStringList());
  addToPathFilterInclude(settings.value("Filter/IncludePathFilter").toStringList());
  addToPathFilterExclude(settings.value("Filter/ExcludePathFilter").toStringList());
  addToAddonFilterInclude(settings.value("Filter/IncludeAddonPathFilter").toStringList());
  addToAddonFilterExclude(settings.value("Filter/ExcludeAddonPathFilter").toStringList());
  addToAirportIcaoFilterInclude(settings.value("Filter/IncludeAirportIcaoFilter").toStringList());
  addToAirportIcaoFilterExclude(settings.value("Filter/ExcludeAirportIcaoFilter").toStringList());
  addToBglObjectFilterInclude(settings.value("Filter/IncludeBglObjectFilter").toStringList());
  addToBglObjectFilterExclude(settings.value("Filter/ExcludeBglObjectFilter").toStringList());
}

bool BglReaderOptions::includeObject(const QString& string, const QList<QRegExp>& filterListInc,
                                     const QList<QRegExp>& filterListExcl) const
{
  if(filterListInc.isEmpty() && filterListExcl.isEmpty())
    return true;

  bool exFound = false;
  for(const QRegExp& iter : filterListExcl)
  {
    if(iter.exactMatch(string))
    {
      exFound = true;
      break;
    }
  }

  if(filterListInc.isEmpty())
    return !exFound;
  else
  {
    bool incFound = false;
    for(const QRegExp& iter : filterListInc)
    {
      if(iter.exactMatch(string))
      {
        incFound = true;
        break;
      }
    }

    if(filterListExcl.isEmpty())
      return incFound;
    else
      return incFound && !exFound;
  }
}

void BglReaderOptions::addToFilter(const QStringList& filters, QList<QRegExp>& filterList)
{
  for(QString f : filters)
  {
    QString newFilter = f.trimmed();
    if(!newFilter.isEmpty())
      filterList.append(QRegExp(newFilter, Qt::CaseInsensitive, QRegExp::Wildcard));
  }
}

QString BglReaderOptions::adaptPath(const QString& filepath) const
{
  QString newFilename = fromNativeSeparator(filepath);
  if(!filepath.endsWith("/"))
    newFilename.append("/");

  return newFilename;
}

QString BglReaderOptions::fromNativeSeparator(const QString& path) const
{
  return QString(path).replace("\\", "/");
}

QStringList BglReaderOptions::fromNativeSeparators(const QStringList& paths) const
{
  QStringList retval;
  for(const QString& p : paths)
    retval.append(fromNativeSeparator(p));
  return retval;
}

QDebug operator<<(QDebug out, const BglReaderOptions& opts)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Options[verbose " << opts.verbose
  << ", sceneryFile \"" << opts.sceneryFile
  << "\", basepath \"" << opts.basepath
  << "\", deletes " << opts.deletes
  << ", incomplete " << opts.incomplete
  << ", debugAutocommit " << opts.autocommit;

  out << ", Include file filter [";
  for(const QRegExp& f : opts.fileFiltersInc)
    out << "pattern" << f.pattern();
  out << "]";
  out << ", Exclude file filter [";
  for(const QRegExp& f : opts.fileFiltersExcl)
    out << "pattern" << f.pattern();
  out << "]";

  out << ", Include path filter [";
  for(const QRegExp& f : opts.pathFiltersInc)
    out << "pattern" << f.pattern();
  out << "]";
  out << ", Exclude path filter [";
  for(const QRegExp& f : opts.pathFiltersExcl)
    out << "pattern" << f.pattern();
  out << "]";

  out << ", Include airport filter [";
  for(const QRegExp& f : opts.airportIcaoFiltersInc)
    out << "pattern" << f.pattern();
  out << "]";
  out << ", Exclude airport filter [";
  for(const QRegExp& f : opts.airportIcaoFiltersExcl)
    out << "pattern" << f.pattern();
  out << "]";

  out << ", Include addon filter [";
  for(const QRegExp& f : opts.addonFiltersInc)
    out << "pattern" << f.pattern();
  out << "]";
  out << ", Exclude addon filter [";
  for(const QRegExp& f : opts.addonFiltersExcl)
    out << "pattern" << f.pattern();
  out << "]";

  out << ", Include type filter [";
  for(type::BglObjectType type : opts.bglObjectTypeFiltersInc)
    out << "pattern" << type::bglObjectTypeToString(type);
  out << "]";
  out << ", Exclude type filter [";
  for(type::BglObjectType type : opts.bglObjectTypeFiltersExcl)
    out << "pattern" << type::bglObjectTypeToString(type);
  out << "]";
  out << "]";
  return out;
}

QString type::bglObjectTypeToString(type::BglObjectType type)
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

    case APRONLIGHT:
      return "APRONLIGHT";

    case FENCE:
      return "FENCE";

    case TAXIWAY:
      return "TAXIWAY";

    case GEOMETRY:
      return "GEOMETRY";

    case UNKNOWN:
      return "UNKNWON";
  }
  return "UNKNWON";
}

type::BglObjectType type::stringToBglObjectType(const QString& typeStr)
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
  else if(typeStr == "APRONLIGHT")
    return APRONLIGHT;
  else if(typeStr == "FENCE")
    return FENCE;
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
