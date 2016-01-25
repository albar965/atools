/*
 * NamelistRecord.cpp
 *
 *  Created on: 21.04.2015
 *      Author: alex
 */

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

Namelist::Namelist(BinaryStream *bs)
  : Record(bs)
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
