/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/nav/ilsvor.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString IlsVor::ilsVorTypeToStr(nav::IlsVorType type)
{
  switch(type)
  {
    case nav::TERMINAL:
      return "T";

    case nav::LOW:
      return "L";

    case nav::HIGH:
      return "H";

    case nav::ILS:
      return "I";

    case nav::VOT:
      return "V";
  }
  qWarning().nospace().noquote() << "Invalid ILS/VOR type " << type;
  return "INVALID";
}

IlsVor::IlsVor(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  type = static_cast<nav::IlsVorType>(bs->readUByte());
}

IlsVor::~IlsVor()
{
}

QDebug operator<<(QDebug out, const IlsVor& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " IlsVor["
  << "type " << IlsVor::ilsVorTypeToStr(record.getType())
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
