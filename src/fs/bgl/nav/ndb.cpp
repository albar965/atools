/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/nav/ndb.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
#endif

QString Ndb::ndbTypeToStr(nav::NdbType type)
{
  switch(type)
  {
    case nav::COMPASS_POINT:
      return "CP";

    case nav::MH:
      return "MH";

    case nav::H:
      return "H";

    case nav::HH:
      return "HH";
  }
  qWarning().nospace().noquote() << "Invalid NDB type " << type;
  return "INVALID";
}

Ndb::Ndb(const NavDatabaseOptions *options, BinaryStream *bs)
  : NavBase(options, bs)
{
  type = static_cast<nav::NdbType>(bs->readShort());
  frequency = bs->readInt() / 10;
  position = BglPosition(bs, true, 1000.f);
  range = bs->readFloat();
  magVar = converter::adjustMagvar(bs->readFloat());
  ident = converter::intToIcao(bs->readUInt());

  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  // Read only name subrecord
  if(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    unsigned int id = r.getId();

    rec::NdbRecordType t = static_cast<rec::NdbRecordType>(id & 0x00ff);
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::NDB_NAME:
        name = bs->readString(r.getSize() - Record::SIZE, encoding);
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in NDB record 0x"
                                       << hex << t << dec << " for ident " << ident;
    }
    r.seekToEnd();
  }
}

Ndb::~Ndb()
{
}

QDebug operator<<(QDebug out, const Ndb& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const NavBase&>(record)
                          << " Ndb[type " << Ndb::ndbTypeToStr(record.type)
                          << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
