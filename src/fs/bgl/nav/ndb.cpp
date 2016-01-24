/*
 * Ndb.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#include "ndb.h"
#include "../bglposition.h"
#include "../converter.h"
#include "../recordtypes.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Ndb::ndbTypeToStr(nav::NdbType type)
{
  switch(type)
  {
    case nav::COMPASS_POINT:
      return "COMPASS_POINT";

    case nav::MH:
      return "MH";

    case nav::H:
      return "H";

    case nav::HH:
      return "HH";
  }
  qWarning().nospace().noquote() << "Unknown NDB type " << type;
  return "";
}

Ndb::Ndb(BinaryStream *bs)
  : NavBase(bs)
{
  type = static_cast<nav::NdbType>(bs->readShort());
  frequency = bs->readInt() / 10;
  position = BglPosition(bs, 1000.f);
  range = bs->readFloat();
  magVar = bs->readFloat();
  ident = converter::intToIcao(bs->readUInt());

  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  while(bs->tellg() < startOffset + size)
  {
    Record r(bs);
    rec::NdbRecordType t = r.getId<rec::NdbRecordType>();

    switch(t)
    {
      case rec::NDB_NAME:
        name = bs->readString(r.getSize() - 6);
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
