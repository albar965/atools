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

#include "fs/db/runwayindex.h"

#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace db {

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
