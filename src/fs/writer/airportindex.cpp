/*
 * AirportIndex.cpp
 *
 *  Created on: 03.05.2015
 *      Author: alex
 */

#include "fs/writer/airportindex.h"

#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace writer {

void AirportIndex::add(const QString& airportIdent, int airportId)
{
  airportIndexMap[airportIdent] = airportId;
}

int AirportIndex::getAirportId(const QString& airportIdent, const QString& sourceObject)
{
  AirportIndexTypeConstIter it = airportIndexMap.find(airportIdent);
  if(it != airportIndexMap.end())
    return it.value();
  else
  {
    qWarning().nospace().noquote() << "Airport ID for ident " << airportIdent << " not found for " <<
    sourceObject;
    return -1;
  }
}

AirportIndex::~AirportIndex()
{
}

} // namespace writer
} // namespace fs
} // namespace atools
