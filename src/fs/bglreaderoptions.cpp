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

bool BglReaderOptions::includePath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), pathFiltersInc, pathFiltersExcl);
}

bool BglReaderOptions::isAddonPath(const QString& filepath) const
{
  return includeObject(adaptPath(filepath), addonFiltersInc, addonFiltersExcl);
}

bool BglReaderOptions::includeFilename(const QString& filename) const
{
  QString fn = QFileInfo(filename).fileName();
  return includeObject(fn, fileFiltersInc, fileFiltersExcl);
}

bool BglReaderOptions::includeAirport(const QString& icao) const
{
  return includeObject(icao, airportIcaoFiltersInc, airportIcaoFiltersExcl);
}

void BglReaderOptions::setFilenameFilterInc(const QStringList& filter)
{
  setFilter(filter, fileFiltersInc);
}

void BglReaderOptions::setAirportIcaoFilterInc(const QStringList& filter)
{
  setFilter(filter, airportIcaoFiltersInc);
}

void BglReaderOptions::setPathFilterInc(const QStringList& filter)
{
  setFilter(toNativeSeparators(filter), pathFiltersInc);
}

void BglReaderOptions::setAddonFilterInc(const QStringList& filter)
{
  setFilter(toNativeSeparators(filter), addonFiltersInc);
}

void BglReaderOptions::setFilenameFilterExcl(const QStringList& filter)
{
  setFilter(filter, fileFiltersExcl);
}

void BglReaderOptions::setAirportIcaoFilterExcl(const QStringList& filter)
{
  setFilter(filter, airportIcaoFiltersExcl);
}

void BglReaderOptions::setPathFilterExcl(const QStringList& filter)
{
  setFilter(toNativeSeparators(filter), pathFiltersExcl);
}

void BglReaderOptions::setAddonFilterExcl(const QStringList& filter)
{
  setFilter(toNativeSeparators(filter), addonFiltersExcl);
}

void BglReaderOptions::setBglObjectFilterInc(const QStringList& filters)
{
  setBglObjectFilter(filters, bglObjectTypeFiltersInc);
}

void BglReaderOptions::setBglObjectFilterExcl(const QStringList& filters)
{
  setBglObjectFilter(filters, bglObjectTypeFiltersExcl);
}

void BglReaderOptions::setBglObjectFilter(const QStringList& filters,
                                          QSet<atools::fs::type::BglObjectType>& filterList)
{
  for(const QString& f : filters)
    if(!f.isEmpty())
      filterList.insert(type::stringToBglObjectType(f));
}

bool BglReaderOptions::includeBglObject(type::BglObjectType type) const
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

void BglReaderOptions::loadFromSettings(const QSettings& settings)
{
  setVerbose(settings.value("Options/Verbose", false).toBool());
  setResolveAirways(settings.value("Options/ResolveAirways", true).toBool());
  setDatabaseReport(settings.value("Options/DatabaseReport", true).toBool());
  setDeletes(settings.value("Options/ProcessDelete", true).toBool());
  setFilterRunways(settings.value("Options/FilterRunways", true).toBool());
  setIncomplete(settings.value("Options/SaveIncomplete", true).toBool());
  setAutocommit(settings.value("Options/Autocommit", false).toBool());

  setFilenameFilterInc(settings.value("Filter/IncludeFilenames").toStringList());
  setFilenameFilterExcl(settings.value("Filter/ExcludeFilenames").toStringList());
  setPathFilterInc(settings.value("Filter/IncludePathFilter").toStringList());
  setPathFilterExcl(settings.value("Filter/ExcludePathFilter").toStringList());
  setAddonFilterInc(settings.value("Filter/IncludeAddonPathFilter").toStringList());
  setAddonFilterExcl(settings.value("Filter/ExcludeAddonPathFilter").toStringList());
  setAirportIcaoFilterInc(settings.value("Filter/IncludeAirportIcaoFilter").toStringList());
  setAirportIcaoFilterExcl(settings.value("Filter/ExcludeAirportIcaoFilter").toStringList());
  setBglObjectFilterInc(settings.value("Filter/IncludeBglObjectFilter").toStringList());
  setBglObjectFilterExcl(settings.value("Filter/ExcludeBglObjectFilter").toStringList());
}

bool BglReaderOptions::includeObject(const QString& filterStr, const QList<QRegExp>& filterListInc,
                                     const QList<QRegExp>& filterListExcl) const
{
  if(filterListInc.isEmpty() && filterListExcl.isEmpty())
    return true;

  bool exFound = false;
  for(const QRegExp& iter : filterListExcl)
    if(iter.exactMatch(filterStr))
    {
      exFound = true;
      break;
    }

  if(filterListInc.isEmpty())
    return !exFound;
  else
  {
    bool incFound = false;
    for(const QRegExp& iter : filterListInc)
      if(iter.exactMatch(filterStr))
      {
        incFound = true;
        break;
      }

    if(filterListExcl.isEmpty())
      return incFound;
    else
      return incFound && !exFound;
  }
}

void BglReaderOptions::setFilter(const QStringList& filters, QList<QRegExp>& filterList)
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
  QString newFilename = filepath;
  if(!filepath.endsWith(QDir::separator()))
    newFilename.append(QDir::separator());

  return toNativeSeparator(newFilename);
}

QString BglReaderOptions::toNativeSeparator(const QString& path) const
{
  return QString(path).replace("\\", "/");
}

QStringList BglReaderOptions::toNativeSeparators(const QStringList& paths) const
{
  QStringList retval;
  for(const QString& p : paths)
    retval.append(toNativeSeparator(p));
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
