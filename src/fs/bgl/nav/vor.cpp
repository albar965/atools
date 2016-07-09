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

#include "fs/bgl/nav/vor.h"
#include "fs/bgl/nav/localizer.h"
#include "fs/bgl/nav/dme.h"
#include "fs/bgl/nav/glideslope.h"
#include "io/binarystream.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

enum VorFlags
{
  // bit 0: if 0 then DME only, otherwise 1 for ILS
  // bit 2: backcourse (0 = false, 1 = true)
  // bit 3: glideslope present
  // bit 4: DME present
  // bit 5: NAV true

  FLAGS_DME_ONLY = 1 << 0,
  FLAGS_BC = 1 << 1,
  FLAGS_GS = 1 << 2,
  FLAGS_DME = 1 << 3,
  FLAGS_NAV = 1 << 4
};

Vor::Vor(const BglReaderOptions *options, BinaryStream *bs)
  : NavBase(options, bs), dme(nullptr)
{
  type = static_cast<nav::IlsVorType>(bs->readUByte());
  int flags = bs->readUByte();

  dmeOnly = (flags & FLAGS_DME_ONLY) == 0;
  // TODO compare flags with record presence
  // hasDme = (flags & FLAGS_DME) == FLAGS_DME;
  // hasNav = (flags & FLAGS_NAV) == FLAGS_NAV;

  position = BglPosition(bs, true, 1000.f);
  frequency = bs->readInt() / 1000;
  range = bs->readFloat();
  magVar = converter::adjustMagvar(bs->readFloat());

  ident = converter::intToIcao(bs->readUInt());

  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);

  // TODO report wiki error ap ident is never set
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(options, bs);
        break;
      case atools::fs::bgl::rec::LOCALIZER:
      case atools::fs::bgl::rec::GLIDESLOPE:
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in VOR record 0x" << hex << t << dec <<
        " for ident " << ident;
    }
    r.seekToEnd();
  }
}

Vor::~Vor()
{
  delete dme;
}

QDebug operator<<(QDebug out, const Vor& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const NavBase&>(record)
  << " Vor["
  << "type " << IlsVor::ilsVorTypeToStr(record.type)
  << ", dmeOnly " << record.dmeOnly;
  if(record.dme != nullptr)
    out << ", " << *record.dme;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
