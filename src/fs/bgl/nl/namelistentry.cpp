/*
 * NamelistIcaoRecord.cpp
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#include "fs/bgl/nl/namelistentry.h"

#include "logging/loggingdefs.h"


namespace atools {
namespace fs {
namespace bgl {

QDebug operator<<(QDebug out, const NamelistEntry& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "NamelistEntry["
  << "airport ID " << record.airportIdent
  << ", airport " << record.airportName
  << ", city " << record.cityName
  << ", state " << record.stateName
  << ", country " << record.countryName
  << ", region ID " << record.regionIdent
  << ", region name " << record.regionName
  << "]";
  return out;
}

NamelistEntry::~NamelistEntry()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
