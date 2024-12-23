/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
#include "fs/bgl/nav/dme.h"
#include "io/binarystream.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
#endif

enum VorFlags
{
  FLAGS_DME_ONLY = 1 << 0, // if 0 then DME only, otherwise 1 for ILS
  FLAGS_BC = 1 << 1, // backcourse (0 = false, 1 = true)
  FLAGS_GS = 1 << 2, // glideslope present
  FLAGS_DME = 1 << 3, // DME present
  FLAGS_NAV = 1 << 4, // NAV true
  FLAGS_TACAN_MSFS = 1 << 1 /* 2020 and 2024 */
};

Vor::Vor(const NavDatabaseOptions *options, BinaryStream *stream)
  : NavBase(options, stream)
{
  type = static_cast<nav::IlsVorType>(stream->readUByte());
  int flags = stream->readUByte();
  tacan = flags & FLAGS_TACAN_MSFS;
  dmeOnly = (flags & FLAGS_DME_ONLY) == 0 && !tacan;
  // hasDme = (flags & FLAGS_DME) == FLAGS_DME;
  // hasNav = (flags & FLAGS_NAV) == FLAGS_NAV;

  // Flag fields found in MSFS 2024
  // "TCN" "ZZ" 0x33 0b110011 0x0 dmeOnly false dummy 0x0
  // "VDM" "ZZ" 0x31 0b110001 0x0 dmeOnly false dummy 0x0
  // "VOR" "ZZ" 0x1  0b000001 0x0 dmeOnly false dummy 0x0
  // "DME" "ZZ" 0x10 0b010000 0x0 dmeOnly true dummy 0x0

  position = BglPosition(stream, true, 1000.f);
  frequency = stream->readInt() / 1000;
  range = stream->readFloat();
  magVar = converter::adjustMagvar(stream->readFloat());
  ident = id == rec::ILS_VOR_MSFS2024 ? converter::intToIcaoLong(stream->readULong()) : converter::intToIcao(stream->readUInt());

  unsigned int regionFlags = stream->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);

  // Airport ident is never set
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);
  atools::io::Encoding encoding = options->getSimulatorType() == FsPaths::MSFS || id == rec::ILS_VOR_MSFS2024 ?
                                  atools::io::UTF8 : atools::io::LATIN1;

  if(id == rec::ILS_VOR_MSFS2024)
    stream->skip(4); // Skip unknown data

  while(stream->tellg() < startOffset + size)
  {
    Record r(options, stream);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = stream->readString(r.getSize() - Record::SIZE, encoding);
        break;

      case rec::DME:
      case rec::DME_MSFS2024:
        r.seekToStart();
        dme = new Dme(options, stream);
        break;

      // Only ILS records - should not appear here
      case rec::LOCALIZER:
      case rec::LOCALIZER_MSFS2024:
      case rec::GLIDESLOPE:
      default:
        qWarning().nospace().noquote() << Q_FUNC_INFO << " Unexpected record type in VOR record 0x" << hex << t << dec
                                       << " for ident " << ident;
        break;
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
