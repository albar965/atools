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

#include "fs/bgl/nl/namelistentry.h"

#include <QDebug>

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
