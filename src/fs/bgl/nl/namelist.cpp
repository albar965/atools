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

#include "fs/bgl/nl/namelist.h"

#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Namelist::Namelist(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  int numRegionNames = stream->readShort();
  int numCountryNames = stream->readShort();
  int numStateNames = stream->readShort();
  int numCityNames = stream->readShort();
  int numAirportNames = stream->readShort();
  int numICAO = stream->readShort();

  int regionListOffset = stream->readInt();
  int countryListOffset = stream->readInt();
  int stateListOffset = stream->readInt();
  int cityListOffset = stream->readInt();
  int airportListOffset = stream->readInt();
  int icaoListOffset = stream->readInt();

  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  // Read all names from the different offsets
  QStringList regions;
  readList(regions, stream, numRegionNames, regionListOffset, encoding);

  QStringList countries;
  readList(countries, stream, numCountryNames, countryListOffset, encoding);

  QStringList states;
  readList(states, stream, numStateNames, stateListOffset, encoding);

  QStringList cities;
  readList(cities, stream, numCityNames, cityListOffset, encoding);

  QStringList airports;
  readList(airports, stream, numAirportNames, airportListOffset, encoding);

  // Goto to the offset that contains the name indexes
  stream->seekg(startOffset + icaoListOffset);

  // Now put the names into NamelistEntrys
  for(int i = 0; i < numICAO; i++)
  {
    NamelistEntry icaoRec;
    icaoRec.regionName = regions.value(stream->readUByte());
    icaoRec.countryName = countries.value(stream->readUByte());
    icaoRec.stateName = states.value(stream->readShort() >> 4);
    icaoRec.cityName = cities.value(stream->readShort());
    icaoRec.airportName = airports.value(stream->readShort());
    icaoRec.airportIdent = converter::intToIcao(stream->readUInt());
    icaoRec.regionIdent = converter::intToIcao(stream->readUInt());

    stream->skip(4); // QMID Level 9 Square.

    entries.append(icaoRec);
  }
}

Namelist::~Namelist()
{
}

void Namelist::readList(QStringList& names, BinaryStream *stream, int numNames, int listOffset, atools::io::Encoding encoding)
{
  stream->seekg(startOffset + listOffset);

  int *indexes = new int[numNames];
  for(int i = 0; i < numNames; i++)
    indexes[i] = stream->readInt();

  qint64 offs = stream->tellg();
  for(int i = 0; i < numNames; i++)
  {
    stream->seekg(offs + indexes[i]);
    names.append(stream->readString(encoding));
  }
  delete[] indexes;
}

QDebug operator<<(QDebug out, const Namelist& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << "Namelist[numIcaoIdent " << record.entries.size() << Qt::endl;
  out << record.entries;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
