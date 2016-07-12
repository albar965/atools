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

#include "fs/bgl/nav/navbase.h"

namespace atools {
namespace fs {
namespace bgl {

QDebug operator<<(QDebug out, const NavBase& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " NavBase[name " << record.name
  << ", ident " << record.ident
  << ", region " << record.region
  << ", airport ID " << record.airportIdent
  << ", frequency " << record.frequency
  << ", " << record.position
  << ", range " << record.range
  << ", magVar " << record.magVar;
  out << "]";
  return out;
}

NavBase::NavBase(const NavDatabaseOptions *options, io::BinaryStream *bs)
  : Record(options, bs), frequency(0.0f), range(0.0f), magVar(0.0f)
{
}

NavBase::~NavBase()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
