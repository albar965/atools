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

#include "fs/bgl/nl/namelist.h"

#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Namelist::Namelist(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  int numRegionNames = bs->readShort();
  int numCountryNames = bs->readShort();
  int numStateNames = bs->readShort();
  int numCityNames = bs->readShort();
  int numAirportNames = bs->readShort();
  int numICAO = bs->readShort();

  int regionListOffset = bs->readInt();
  int countryListOffset = bs->readInt();
  int stateListOffset = bs->readInt();
  int cityListOffset = bs->readInt();
  int airportListOffset = bs->readInt();
  int icaoListOffset = bs->readInt();

  // Read all names from the different offsets
  QStringList regions;
  readList(regions, bs, numRegionNames, regionListOffset);

  QStringList countries;
  readList(countries, bs, numCountryNames, countryListOffset);

  QStringList states;
  readList(states, bs, numStateNames, stateListOffset);

  QStringList cities;
  readList(cities, bs, numCityNames, cityListOffset);

  QStringList airports;
  readList(airports, bs, numAirportNames, airportListOffset);

  // Goto to the offset that contains the name indexes
  bs->seekg(startOffset + icaoListOffset);

  // Now put the names into NamelistEntrys
  for(int i = 0; i < numICAO; i++)
  {
    NamelistEntry icaoRec;
    icaoRec.regionName = regions.value(bs->readUByte());
    icaoRec.countryName = countries.value(bs->readUByte());
    icaoRec.stateName = states.value(bs->readShort() >> 4);
    icaoRec.cityName = cities.value(bs->readShort());
    icaoRec.airportName = airports.value(bs->readShort());
    icaoRec.airportIdent = converter::intToIcao(bs->readUInt());
    icaoRec.regionIdent = converter::intToIcao(bs->readUInt());

    bs->skip(4); // QMID Level 9 Square.

    entries.append(icaoRec);
  }
}

Namelist::~Namelist()
{
}

void Namelist::readList(QStringList& names, BinaryStream *bs, int numNames, int listOffset)
{
  bs->seekg(startOffset + listOffset);

  int *indexes = new int[numNames];
  for(int i = 0; i < numNames; i++)
    indexes[i] = bs->readInt();

  qint64 offs = bs->tellg();
  for(int i = 0; i < numNames; i++)
  {
    bs->seekg(offs + indexes[i]);
    names.append(bs->readString());
  }
  delete[] indexes;
}

QDebug operator<<(QDebug out, const Namelist& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << "Namelist[numIcaoIdent " << record.entries.size() << endl;
  out << record.entries;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
