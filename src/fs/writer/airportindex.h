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

#ifndef WRITER_AIRPORTINDEX_H_
#define WRITER_AIRPORTINDEX_H_

#include <QHash>

namespace atools {
namespace fs {
namespace writer {

class AirportIndex
{
public:
  AirportIndex()
  {
  }

  virtual ~AirportIndex();

  void add(const QString& airportIdent, int airportId);

  int getAirportId(const QString& airportIdent, const QString& sourceObject);

  void clear()
  {
    airportIndexMap.clear();
  }

private:
  typedef QString AirportIndexKeyType;
  typedef QHash<atools::fs::writer::AirportIndex::AirportIndexKeyType, int> AirportIndexType;
  typedef atools::fs::writer::AirportIndex::AirportIndexType::const_iterator AirportIndexTypeConstIter;

  atools::fs::writer::AirportIndex::AirportIndexType airportIndexMap;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AIRPORTINDEX_H_ */
