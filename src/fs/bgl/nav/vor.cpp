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

enum VorFlags : quint8
{
  FLAGS_DME_ONLY = 1 << 0, // if 0 then DME only, otherwise 1 for ILS
  FLAGS_MSFS_TACAN_VORTAC_MSFS = 1 << 1,
  FLAGS_MSFS_NAV = 1 << 4, // NAV true
};

Vor::Vor(const NavDatabaseOptions *options, BinaryStream *stream)
  : NavBase(options, stream)
{
  type = static_cast<nav::IlsVorType>(stream->readUByte());

  int flags = stream->readUByte();
  dmeOnly = tacan = vortac = false;
  if(options->getSimulatorType() == atools::fs::FsPaths::MSFS)
  {
    // TACAN  "LDK" 0b010010 0x12
    // VORTAC "RQZ" 0b110011 0x33
    // VORDME "GAD" 0b110001 0x31
    // VOR    "MTR" 0b000001 0x01
    // DME    "DCU" 0b010000 0x10

    if(flags & FLAGS_MSFS_TACAN_VORTAC_MSFS)
    {
      if((flags & FLAGS_DME_ONLY) == 0)
        tacan = true;
      else
        vortac = true;
    }
    else
      dmeOnly = (flags & FLAGS_DME_ONLY) == 0;
  }
  else
    dmeOnly = (flags & FLAGS_DME_ONLY) == 0;

  position = BglPosition(stream, true, 1000.f);
  frequency = stream->readInt() / 1000;
  range = stream->readFloat();
  magVar = converter::adjustMagvar(stream->readFloat());
  ident = id == rec::ILS_VOR_MSFS2024 ? converter::intToIcaoLong(stream->readULong()) : converter::intToIcao(stream->readUInt());

#ifdef DEBUG_DUMP_NAVAID_FLAGS
  if(ident == "RQZ")
    qDebug().nospace() << Q_FUNC_INFO << " VORTAC " << ident << " 0b" << bin << flags << Qt::hex << " 0x" << flags;

  if(ident == "LDK")
    qDebug().nospace() << Q_FUNC_INFO << " TACAN " << ident << " 0b" << bin << flags << Qt::hex << " 0x" << flags;

  if(ident == "DCU")
    qDebug().nospace() << Q_FUNC_INFO << " DME " << ident << " 0b" << bin << flags << Qt::hex << " 0x" << flags;

  if(ident == "GAD")
    qDebug().nospace() << Q_FUNC_INFO << " VORDME " << ident << " 0b" << bin << flags << Qt::hex << " 0x" << flags;

  if(ident == "MTR")
    qDebug().nospace() << Q_FUNC_INFO << " VOR " << ident << " 0b" << bin << flags << Qt::hex << " 0x" << flags << position.getPos();
#endif

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
        qWarning().nospace().noquote() << Q_FUNC_INFO << " Unexpected record type in VOR record 0x" << Qt::hex << t << Qt::dec
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
