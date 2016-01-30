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

#include "fs/bgl/nl/namelist.h"
#include "io/binarystream.h"

#include "fs/bgl/converter.h"

#include "fs/bgl/nl/namelistentry.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

void Namelist::readList(QStringList& names, BinaryStream *bs, int numNames, int listOffset)
{
  bs->seekg(startOffset + listOffset);

  int indexes[numNames];
  for(int i = 0; i < numNames; i++)
    indexes[i] = bs->readInt();

  int offs = bs->tellg();
  for(int i = 0; i < numNames; i++)
  {
    bs->seekg(offs + indexes[i]);
    names.push_back(bs->readString());
  }
}

Namelist::Namelist(const BglReaderOptions *options, BinaryStream *bs)
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

  bs->seekg(startOffset + icaoListOffset);

  for(int i = 0; i < numICAO; i++)
  {
    NamelistEntry icaoRec;
    icaoRec.regionName = regions.at(bs->readByte());
    icaoRec.countryName = countries.at(bs->readByte());
    icaoRec.stateName = states.at(bs->readShort() >> 4);
    icaoRec.cityName = cities.at(bs->readShort());
    icaoRec.airportName = airports.at(bs->readShort());
    icaoRec.airportIdent = converter::intToIcao(bs->readUInt());
    icaoRec.regionIdent = converter::intToIcao(bs->readUInt());

    bs->skip(4);

    entries.push_back(icaoRec);
  }
}

Namelist::~Namelist()
{
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
