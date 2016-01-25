/*
 * Vor.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

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

Vor::Vor(BinaryStream *bs)
  : NavBase(bs), dme(nullptr)
{
  type = static_cast<nav::IlsVorType>(bs->readByte());
  int flags = bs->readByte();

  dmeOnly = (flags & FLAGS_DME_ONLY) == 0;
  // hasDme = (flags & FLAGS_DME) == FLAGS_DME;
  // hasNav = (flags & FLAGS_NAV) == FLAGS_NAV;

  position = BglPosition(bs, 1000.f);
  frequency = bs->readInt() / 1000;
  range = bs->readFloat();
  magVar = bs->readFloat();

  ident = converter::intToIcao(bs->readUInt());

  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);

  // TODO report wiki error ap ident is never set
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  while(bs->tellg() < startOffset + size)
  {
    Record r(bs);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = bs->readString(r.getSize() - 6);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(bs);
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in VOR record 0x" << hex << t << dec <<
        " for ident "
                                       << ident;
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
