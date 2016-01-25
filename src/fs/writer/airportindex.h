/*
 * AirportIndex.h
 *
 *  Created on: 03.05.2015
 *      Author: alex
 */

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
