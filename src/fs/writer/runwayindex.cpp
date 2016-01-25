/*
 * RunwayIndex.cpp
 *
 *  Created on: 03.05.2015
 *      Author: alex
 */

#include "fs/writer/runwayindex.h"

#include "logging/loggingdefs.h"


namespace atools {
namespace fs {
namespace writer {

using std::make_pair;

void RunwayIndex::add(const QString& airportIdent, const QString& runwayName, int runwayEndId)
{
  runwayIndexMap[RunwayIndexKeyType(airportIdent, runwayName)] = runwayEndId;
}

int RunwayIndex::getRunwayEndId(const QString& airportIdent,
                                const QString& runwayName,
                                const QString& sourceObject)
{
  RunwayIndexKeyType key(airportIdent, runwayName);

  RunwayIndexTypeConstIter it = runwayIndexMap.find(key);
  if(it != runwayIndexMap.end())
    return it.value();
  else
  {
    qWarning().nospace().noquote() << "Runway end ID for airport " << airportIdent << " and runway " <<
    runwayName
                                   << " not found for " << sourceObject;
    return -1;
  }
}

RunwayIndex::~RunwayIndex()
{
}

} // namespace writer
} // namespace fs
} // namespace atools
