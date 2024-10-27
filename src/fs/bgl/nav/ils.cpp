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

#include "fs/bgl/nav/ils.h"
#include "fs/bgl/nav/localizer.h"
#include "fs/bgl/nav/dme.h"
#include "fs/bgl/nav/glideslope.h"

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

enum IlsFlags
{
  FLAGS_DME_ONLY = 1 << 0,
  FLAGS_BC = 1 << 2,
  FLAGS_GS = 1 << 3,
  FLAGS_DME = 1 << 4,
  FLAGS_NAV = 1 << 5
};

Ils::Ils(const NavDatabaseOptions *options, BinaryStream *stream)
  : NavBase(options, stream), localizer(nullptr), glideslope(nullptr), dme(nullptr)
{
  stream->readUByte();
  int flags = stream->readUByte();

  backcourse = (flags & FLAGS_BC) == FLAGS_BC;
  // TODO  compare values with record presence
  // dmeOnlyOrIls = (flags & FLAGS_DME_ONLY) == FLAGS_DME_ONLY;
  // hasGlideslope = (flags & FLAGS_GS) == FLAGS_GS;
  // hasDme = (flags & FLAGS_DME) == FLAGS_DME;
  // hasNav = (flags & FLAGS_NAV) == FLAGS_NAV;

  // ILS transmitter position
  position = BglPosition(stream, true, 1000.f);
  frequency = stream->readInt() / 1000;
  range = stream->readFloat();
  magVar = converter::adjustMagvar(stream->readFloat());

  // ILS ident
  ident = converter::intToIcao(stream->readUInt());

  unsigned int regionFlags = stream->readUInt();
  // Two letter region code
  region = converter::intToIcao(regionFlags & 0x7ff, true); // TODO wiki region is never set
  // Read airport ICAO ident
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  // Read all subrecords of ILS
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
      case rec::LOCALIZER:
        // This is actually not optional for an ILS
        r.seekToStart();
        localizer = new Localizer(options, stream);
        break;
      case rec::GLIDESLOPE:
        r.seekToStart();
        glideslope = new Glideslope(options, stream);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(options, stream);
        break;
      default:
        qWarning().nospace().noquote() << Q_FUNC_INFO << " Unexpected record type in ILS record 0x"
                                       << hex << t << dec << " for ident " << ident;
    }
    r.seekToEnd();
  }
}

QString Ils::getType() const
{
  // ILS Localizer only, no glideslope   0
  // ILS Localizer/MLS/GLS Unknown cat   U
  // ILS Localizer/MLS/GLS Cat I         1
  // ILS Localizer/MLS/GLS Cat II        2
  // ILS Localizer/MLS/GLS Cat III       3
  // IGS Facility                        I
  // LDA Facility with glideslope        L
  // LDA Facility no glideslope          A
  // SDF Facility with glideslope        S
  // SDF Facility no glideslope          F

  if(glideslope == nullptr)
    return "0"; // Localizer
  else
  {
    QString upName = name.toUpper();
    if(upName.contains("CAT-III") || upName.contains("CAT III") || upName.contains("CATIII"))
      return "3";
    else if(upName.contains("CAT-II") || upName.contains("CAT II") || upName.contains("CATII"))
      return "2";
    else if(upName.contains("CAT-I") || upName.contains("CAT I") || upName.contains("CATI"))
      return "1";
    else
      return "U"; // Unknown category
  }
}

Ils::~Ils()
{
  delete localizer;
  delete dme;
  delete glideslope;
}

QDebug operator<<(QDebug out, const Ils& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const NavBase&>(record)
                          << " Ils[backcourse " << record.backcourse;
  if(record.localizer != nullptr)
    out << ", " << *record.localizer;
  if(record.glideslope != nullptr)
    out << ", " << *record.glideslope;
  if(record.dme != nullptr)
    out << ", " << *record.dme;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
