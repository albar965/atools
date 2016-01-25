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

#include <QList>
#include "logging/loggingdefs.h"

namespace atools {
namespace fs {

BglReaderOptions::BglReaderOptions()
{
}

bool BglReaderOptions::doesFilenameMatch(const QString& filename) const
{
  if(fileFilterRegexp.isEmpty())
    return true;

  for(const QRegularExpression& iter : fileFilterRegexp)
    if(iter.match(filename).hasMatch())
      return true;

  return false;
}

bool BglReaderOptions::doesAirportIcaoMatch(const QString& icao) const
{
  if(airportIcaoFilterRegexp.empty())
    return true;

  for(const QRegularExpression& iter : airportIcaoFilterRegexp)
    if(iter.match(icao).hasMatch())
      return true;

  return false;
}

QDebug operator<<(QDebug out, const BglReaderOptions& opts)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Options[verbose " << opts.verbose
  << ", sceneryFile \"" << opts.sceneryFile
  << "\", basepath \"" << opts.basepath
  << "\", noDeletes " << opts.noDeletes
  << ", noIncomplete " << opts.noIncomplete
  << ", debugAutocommit " << opts.debugAutocommit;

  out << ", File filter [";
  out << opts.fileFilterRegexpStr;
  out << "]";

  out << ", Airport filter [";
  out << opts.airportIcaoFilterRegexpStr;
  out << "]";
  out << "]";
  return out;
}

} // namespace fs
} // namespace atools
