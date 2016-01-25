/*
 * BglReaderOptions.cpp
 *
 *  Created on: 21.05.2015
 *      Author: alex
 */

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
