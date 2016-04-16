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

#include "fs/bgl/nav/airwayentry.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString AirwayEntry::airwayTypeToStr(nav::AirwayType type)
{
  switch(type)
  {
    case nav::NONE:
      return "NONE";

    case nav::VICTOR:
      return "VICTOR";

    case nav::JET:
      return "JET";

    case nav::BOTH:
      return "BOTH";
  }
  qWarning().nospace().noquote() << "Unknown airway type " << type;
  return QString();
}

AirwayEntry::AirwayEntry(const atools::fs::BglReaderOptions *options, BinaryStream *bs)
  : BglBase(options, bs)
{
  type = static_cast<nav::AirwayType>(bs->readUByte());
  name = bs->readString(8);

  next = AirwayWaypoint(options, bs);
  previous = AirwayWaypoint(options, bs);
}

AirwayEntry::~AirwayEntry()
{
}

bool AirwayEntry::hasNextWaypoint() const
{
  return next.getType() != nav::AIRWAY_WP_NONE;
}

bool AirwayEntry::hasPreviousWaypoint() const
{
  return previous.getType() != nav::AIRWAY_WP_NONE;
}

QDebug operator<<(QDebug out, const AirwayEntry& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << " AirwayEntry[type " << AirwayEntry::airwayTypeToStr(record.type)
  << ", name " << record.name;

  if(record.hasNextWaypoint())
    out << ", next " << record.next;
  if(record.hasPreviousWaypoint())
    out << ", previous " << record.previous;

  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
