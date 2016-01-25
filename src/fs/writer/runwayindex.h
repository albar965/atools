/*
 * RunwayIndex.h
 *
 *  Created on: 03.05.2015
 *      Author: alex
 */

#ifndef WRITER_RUNWAYINDEX_H_
#define WRITER_RUNWAYINDEX_H_

#include <QHash>

namespace atools {
namespace fs {
namespace writer {

class RunwayIndex
{
public:
  RunwayIndex()
  {
  }

  virtual ~RunwayIndex();

  void add(const QString& airportIdent, const QString& runwayName, int runwayEndId);

  int getRunwayEndId(const QString& airportIdent, const QString& runwayName, const QString& sourceObject);

  void clear()
  {
    runwayIndexMap.clear();
  }

private:
  typedef QPair<QString, QString> RunwayIndexKeyType;
  typedef QHash<atools::fs::writer::RunwayIndex::RunwayIndexKeyType, int> RunwayIndexType;
  typedef atools::fs::writer::RunwayIndex::RunwayIndexType::const_iterator RunwayIndexTypeConstIter;

  atools::fs::writer::RunwayIndex::RunwayIndexType runwayIndexMap;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_RUNWAYINDEX_H_ */
