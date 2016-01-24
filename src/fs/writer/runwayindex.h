/*
 * RunwayIndex.h
 *
 *  Created on: 03.05.2015
 *      Author: alex
 */

#ifndef WRITER_RUNWAYINDEX_H_
#define WRITER_RUNWAYINDEX_H_

#include <QHash>
#include <QString>

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
  typedef QHash<RunwayIndexKeyType, int> RunwayIndexType;
  typedef RunwayIndexType::const_iterator RunwayIndexTypeConstIter;

  RunwayIndexType runwayIndexMap;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_RUNWAYINDEX_H_ */
